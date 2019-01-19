/*
 * mcv4fwdd: IPv4 Multicast Forwarding Daemon
 * Copyright (C) 2018  Niels Penneman
 *
 * This file is part of mcv4fwdd.
 *
 * mcv4fwdd is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Affero General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * mcv4fwdd is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Affero General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with mcv4fwdd. If not, see <https://www.gnu.org/licenses/>.
 */


#include <cstdio>
#include <iostream>
#include <map>

#include <ifaddrs.h>
#include <syslog.h>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include "commandline.h"
#include "router.h"
#include "utility.h"
#include "config/model/configuration.h"
#include "config/parser/parser.h"


using io_service = boost::asio::io_service;
using Configuration = config::model::Configuration;
using Network = config::model::Network;
using InterfaceAddressMap = std::multimap<std::string, Network>;


namespace
{
  /** Performs the double fork trick to run as a daemon */
  int daemonize(const std::string &pidFileName, io_service &ioService, bool &exit);

  /** Builds a map of interface names to their IPv4 networks */
  InterfaceAddressMap getInterfaceAddresses();

  std::unique_ptr<Configuration> loadConfiguration(const std::string &filename);

  /** Translates the parsed configuration file into a run-time configuration for the router */
  int setupRouter(Configuration &configuration, const InterfaceAddressMap &interfaceAddresses, Router &router);


  int daemonize(const std::string &pidFileName, io_service &ioService, bool &exit)
  {
    exit = true;

    // Primary fork
    ioService.notify_fork(io_service::fork_prepare);
    if (pid_t pid = fork())
    {
      if (pid > 0)
      {
        ioService.notify_fork(io_service::fork_parent);
        return 0;
      }
      else
      {
        syslog(LOG_ERR, "Primary fork failed: %m");
        return 1;
      }
    }
    ioService.notify_fork(io_service::fork_child);

    if (setsid() < 0)
    {
      syslog(LOG_ERR, "setsid() failed: %m");
      return 1;
    }
    if (chdir("/") < 0)
    {
      syslog(LOG_ERR, "chdir(\"/\") failed: %m");
      return 1;
    }
    umask(0);

    // Secondary fork
    ioService.notify_fork(io_service::fork_prepare);
    if (pid_t pid = fork())
    {
      if (pid > 0)
      {
        ioService.notify_fork(io_service::fork_parent);
        return 0;
      }
      else
      {
        syslog(LOG_ERR, "Secondary fork failed: %m");
        return 1;
      }
    }
    ioService.notify_fork(io_service::fork_child);

    // Redirect all I/O to /dev/null
    {
      int fd = open("/dev/null", O_RDWR);
      if (fd < 0)
      {
        syslog(LOG_ERR, "Failed to open /dev/null: %m");
        return 1;
      }
      for (auto fileNo: { STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO })
      {
        if (dup2(fd, fileNo) < 0)
        {
          syslog(LOG_ERR, "Failed to reassign fd=%d: %m", fileNo);
          return 1;
        }
      }
      close(fd);
    }

    // Write PID file; on success, leave it open and locked
    {
      int fd = open(pidFileName.c_str(), O_CREAT | O_TRUNC | O_WRONLY , 0640);
      if (fd < 0)
      {
        syslog(LOG_ERR, "Failed to open PID file \"%s\": %m", pidFileName.c_str());
        assert(false);
        return 1;
      }
      if (lockf(fd, F_TLOCK, 0) < 0)
      {
        syslog(LOG_ERR, "Failed to lock PID file \"%s\": %m", pidFileName.c_str());
        assert(false);
        return 1;
      }
      char str[std::numeric_limits<pid_t>::digits10 + 4];
      errno = 0;
      auto printed = snprintf(str, sizeof(str), "%d\n", getpid());
      if (printed < 0 || static_cast<size_t>(printed) >= sizeof(str))
      {
        syslog(LOG_ERR, "Failed to format PID file \"%s\": %m", pidFileName.c_str());
        assert(false);
        close(fd);
        return 1;
      }
      errno = 0;
      auto written = write(fd, str, printed);
      if (written < 0 || written != printed)
      {
        syslog(LOG_ERR, "Failed to write to PID file \"%s\": %m", pidFileName.c_str());
        assert(false);
        close(fd);
        return 1;
      }
    }

    exit = false;
    return 0;
  }

  InterfaceAddressMap getInterfaceAddresses()
  {
    ifaddrs *interfaceAddressList = nullptr;
    if (getifaddrs(&interfaceAddressList) == -1)
    {
      assert(!interfaceAddressList);
      throw std::runtime_error(utility::getErrorString(errno));
    }

    InterfaceAddressMap map;
    for (const ifaddrs *item = interfaceAddressList; item != nullptr; item = item->ifa_next)
    {
      if (item->ifa_addr && item->ifa_addr->sa_family == AF_INET && item->ifa_netmask->sa_family == AF_INET)
      {
        boost::asio::ip::address_v4 address(ntohl(
          reinterpret_cast<const sockaddr_in *>(item->ifa_addr)->sin_addr.s_addr));
        auto prefixLength = __builtin_popcountl(ntohl(
          reinterpret_cast<const sockaddr_in *>(item->ifa_netmask)->sin_addr.s_addr));
        map.emplace(item->ifa_name, Network(address, prefixLength, false));
      }
    }

    freeifaddrs(interfaceAddressList);
    return std::move(map);
  }

  std::unique_ptr<Configuration> loadConfiguration(const std::string &filename)
  {
    utility::UniqueFilePtr file(fopen(filename.c_str(), "r"));
    if (!file)
    {
      auto error = utility::getErrorString(errno);
      std::cerr << "Error opening configuration file '" << filename << "': " << error << std::endl;
      return nullptr;
    }
    auto configuration = std::make_unique<Configuration>();
    if (!config::parser::parse(filename, file, *configuration))
    {
      return nullptr;
    }
#ifndef NDEBUG
    std::cout << *configuration << std::endl;
#endif
    return std::move(configuration);
  }

  int setupRouter(Configuration &configuration, const InterfaceAddressMap &interfaceAddresses, Router &router)
  {
    for (const auto &serviceConfiguration: configuration.getServiceConfigurations())
    {
      boost::asio::ip::udp::endpoint multicastEndpoint(serviceConfiguration.getGroupAddress(),
        serviceConfiguration.getPort());
      for (const auto &forwardingRule: serviceConfiguration.getForwardingRules())
      {
        // Figure out source interface address
        auto fromInterfaceAddress = interfaceAddresses.find(forwardingRule.getFromInterface());
        if (fromInterfaceAddress == std::end(interfaceAddresses))
        {
          std::cerr << "Failed to identify IPv4 network for interface " << forwardingRule.getFromInterface() << std::endl;
          return 1;
        }
        else
        {
          auto next = std::next(fromInterfaceAddress);
          if (next != std::end(interfaceAddresses) && next->first == forwardingRule.getFromInterface())
          {
            std::cerr << "Warning: interface " << forwardingRule.getFromInterface()
              << " has multiple IPv4 addresses; joining receiver on " << fromInterfaceAddress->second << std::endl;
          }
        }

        // Figure out destination interface address
        auto toInterfaceAddress = interfaceAddresses.find(forwardingRule.getToInterface());
        if (toInterfaceAddress == std::end(interfaceAddresses))
        {
          std::cerr << "Failed to identify IPv4 network for interface " << forwardingRule.getToInterface() << std::endl;
          return 1;
        }
        else
        {
          auto next = std::next(toInterfaceAddress);
          if (next != std::end(interfaceAddresses) && next->first == forwardingRule.getToInterface())
          {
            std::cerr << "Warning: interface " << forwardingRule.getToInterface()
              << " has multiple IPv4 addresses; configuring sender on " << toInterfaceAddress->second << std::endl;
          }
        }

        // Figure out from which addresses we need to forward datagrams
        std::list<Network> fromInterfaceAcceptedAddresses;
        if (forwardingRule.getNetworks().empty())
        {
          // No configuration; accept from all networks on which this interface has addresses (can contain duplicates)
          auto iter = fromInterfaceAddress;
          do
          {
            fromInterfaceAcceptedAddresses.emplace_back(fromInterfaceAddress->second.getMaskedNetwork());
            iter = std::next(iter);
          }
          while (iter != std::end(interfaceAddresses) && iter->first == fromInterfaceAddress->first);
        }
        else
        {
          // Add only those networks specified in the configuration
          std::copy(std::begin(forwardingRule.getNetworks()), std::end(forwardingRule.getNetworks()),
            std::back_inserter(fromInterfaceAcceptedAddresses));
        }

        router.addRule(multicastEndpoint, fromInterfaceAddress->second.getAddress(),
          fromInterfaceAcceptedAddresses, toInterfaceAddress->second.getAddress());
      }
    }
    return 0;
  }
}

int main(int argc, char* argv[])
{
  CommandLine commandLine;
  if (int r = commandLine.parse(argc, argv))
  {
    return r;
  }

  auto configuration = loadConfiguration(commandLine.getConfigurationFileName());
  if (!configuration)
  {
    return 1;
  }

  InterfaceAddressMap interfaceAddresses;
  try
  {
    interfaceAddresses = getInterfaceAddresses();
  }
  catch (std::runtime_error &e)
  {
    std::cerr << "Failed to query network interface addresses: " << e.what() << std::endl;
    return 1;
  }

  io_service ioService;
  Router router(ioService);
  if (int r = setupRouter(*configuration, interfaceAddresses, router))
  {
    return r;
  }
  configuration.reset();
  router.start();
  if (commandLine.getTestConfigurationOnly())
  {
    return 0;
  }

  // Cleanly exit on SIGINT (CTRL-C) and SIGTERM
  boost::asio::signal_set signals(ioService, SIGINT, SIGTERM);
  signals.async_wait(boost::bind(&io_service::stop, &ioService));

  openlog(argv[0], LOG_PID | (commandLine.getForeground() ? LOG_PERROR : 0), LOG_USER);
  if (!commandLine.getForeground())
  {
    bool exit = true;
    int r = daemonize(commandLine.getPidFileName(), ioService, exit);
    if (exit)
    {
      return r;
    }
  }

  ioService.run();
  return 0;
}
