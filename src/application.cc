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


#include "application.h"

#include <iostream>

#include <ifaddrs.h>
#include <syslog.h>
#include <boost/bind.hpp>

#include "utility.h"


namespace
{
  using InterfaceAddressMap = Application::InterfaceAddressMap;
  using Network = config::model::Network;


  const auto RESET_DELAY = boost::posix_time::seconds(5);

  /** Returns true when the given set of interface names refers only to interfaces which are up */
  bool areAllInterfacesUp(const std::set<std::string> &interfaces);

  /** Builds a list of senders (as IP networks) whose datagrams will be forwarded according to the given rule */
  std::list<Network> getAcceptedSourceNetworks(const config::model::ForwardingRule &forwardingRule,
    InterfaceAddressMap::const_iterator sourceIter, const InterfaceAddressMap &interfaceAddresses);

  /** Builds a map of interface names to their IPv4 networks */
  InterfaceAddressMap getInterfaceAddresses();

  /** Finds the address for the given interfaces; throws if none found */
  InterfaceAddressMap::const_iterator getInterfaceAddress(const InterfaceAddressMap &interfaceAddresses,
    const std::string &interface, const char *purpose);

  /** Checks whether the given interface is up using the given socket descriptor */
  bool isInterfaceUp(int socketFD, const std::string &interface);

  void warnOnMultipleAddressesForInterface(const InterfaceAddressMap &interfaceAddresses,
    const std::string &interface, InterfaceAddressMap::const_iterator iter, const char *purpose);


  bool areAllInterfacesUp(const std::set<std::string> &interfaces)
  {
    std::ostringstream allInterfacesOS;
    allInterfacesOS << "Checking whether interfaces are up:";
    for (const std::string &interface: interfaces)
    {
      allInterfacesOS << ' ' << interface;
    }
    syslog(LOG_INFO, allInterfacesOS.str().c_str());

    bool allInterfacesUp = true;
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock == -1)
    {
      throw std::runtime_error(utility::getErrorString(errno));
    }
    for (const std::string &interface: interfaces)
    {
      if (!isInterfaceUp(sock, interface))
      {
        syslog(LOG_NOTICE, "Required interface '%s' is down", interface.c_str());
        allInterfacesUp = false;
      }
    }
    close(sock);
    return allInterfacesUp;
  }

  std::list<Network> getAcceptedSourceNetworks(const config::model::ForwardingRule &forwardingRule,
    InterfaceAddressMap::const_iterator sourceIter, const InterfaceAddressMap &interfaceAddresses)
  {
    std::list<Network> networks;
    if (forwardingRule.getNetworks().empty())
    {
      // No configuration; accept from all networks on which this interface has addresses (can contain duplicates)
      auto iter = sourceIter;
      do
      {
        networks.emplace_back(sourceIter->second.getMaskedNetwork());
        iter = std::next(iter);
      }
      while (iter != std::end(interfaceAddresses) && iter->first == sourceIter->first);
    }
    else
    {
      // Add only those networks specified in the configuration
      std::copy(std::begin(forwardingRule.getNetworks()), std::end(forwardingRule.getNetworks()),
        std::back_inserter(networks));
    }
    return std::move(networks);
  }

  InterfaceAddressMap getInterfaceAddresses()
  {
    ifaddrs *interfaceAddressList = nullptr;
    if (getifaddrs(&interfaceAddressList) == -1)
    {
      assert(!interfaceAddressList);
      auto error = errno;
      std::ostringstream oss;
      oss << "Failed to query network interface addresses: " << utility::getErrorString(error);
      throw std::runtime_error(oss.str());
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

  InterfaceAddressMap::const_iterator getInterfaceAddress(const InterfaceAddressMap &interfaceAddresses,
    const std::string &interface, const char *purpose)
  {
    auto addressIter = interfaceAddresses.find(interface);
    if (addressIter == std::end(interfaceAddresses))
    {
      std::ostringstream oss;
      oss << "Failed to identify IPv4 network for interface " << interface << std::endl;
      throw std::runtime_error(oss.str());
    }
    warnOnMultipleAddressesForInterface(interfaceAddresses, interface, addressIter, purpose);
    return addressIter;
  }

  bool isInterfaceUp(int socketFD, const std::string &interface)
  {
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, interface.c_str());
    if (ioctl(socketFD, SIOCGIFFLAGS, &ifr) < 0)
    {
      auto error = errno;
      if (error == ENODEV)
      {
        // Interface does not exist (yet?); consider it down
        return false;
      }
      throw std::runtime_error(utility::getErrorString(error));
    }
    return !!(ifr.ifr_flags & IFF_UP);
  }

  void warnOnMultipleAddressesForInterface(const InterfaceAddressMap &interfaceAddresses,
    const std::string &interface, InterfaceAddressMap::const_iterator iter, const char *purpose)
  {
    auto next = std::next(iter);
    if (next != std::end(interfaceAddresses) && next->first == interface)
    {
      std::ostringstream oss;
      oss << "Warning: interface " << interface << " has multiple IPv4 addresses; " << purpose << " on "
        << iter->second;
      syslog(LOG_WARNING, "%s", oss.str().c_str());
    }
  }
}

Application::Application(std::unique_ptr<Configuration> &&configuration):
  m_configuration(std::move(configuration)),
  m_ioService(),
  m_resetTimer(),
  m_router()
{}

void Application::doRestart(const boost::system::error_code &error)
{
  // Only restart if timer was not canceled
  if (!error)
  {
    setupRouter();
  }
}

int Application::run()
{
  m_ioService = std::make_shared<io_service>();
  m_resetTimer = std::make_unique<deadline_timer>(*m_ioService);

  // Cleanly exit on SIGINT (CTRL-C) and SIGTERM
  boost::asio::signal_set signals(*m_ioService, SIGINT, SIGTERM);
  signals.async_wait(boost::bind(&io_service::stop, m_ioService));

  m_ioService->post(boost::bind(&Application::setupRouter, this));

  try
  {
    m_ioService->run();
  }
  catch (const std::runtime_error &e)
  {
    syslog(LOG_ALERT, "crashed: %s", e.what());
    return 1;
  }
  return 0;
}

int Application::run(std::unique_ptr<Configuration> &&configuration)
{
  return Application(std::move(configuration)).run();
}

void Application::scheduleRestart()
{
  m_resetTimer->expires_from_now(RESET_DELAY);
  m_resetTimer->async_wait(boost::bind(&Application::doRestart, this, boost::asio::placeholders::error));
}

void Application::setupRouter()
{
  m_router.reset();

  auto interfaces = m_configuration->getInterfaces();

  if (!areAllInterfacesUp(interfaces))
  {
    if (m_resetTimer != nullptr)
    {
      scheduleRestart();
    }
    return;
  }

  // All interfaces should be up at this point
  auto interfaceAddresses = getInterfaceAddresses();
  if (m_ioService != nullptr)
  {
    m_router = std::make_unique<Router>(*m_ioService);
  }

  try
  {
    setupRouterConfiguration(interfaceAddresses);
  }
  catch (const std::runtime_error &e)
  {
    if (m_ioService == nullptr)
    {
      // Just testing; handle exception in test method
      throw;
    }
    syslog(LOG_ERR, "router configuration failed: %s", e.what());
    scheduleRestart();
    return;
  }
  if (m_router != nullptr)
  {
    m_router->start();
  }
}

void Application::setupRouterConfiguration(const InterfaceAddressMap &interfaceAddresses)
{
  for (const auto &serviceConfiguration: m_configuration->getServiceConfigurations())
  {
    setupRouterConfiguration(serviceConfiguration, interfaceAddresses);
  }
}

void Application::setupRouterConfiguration(const ServiceConfiguration &serviceConfiguration,
  const InterfaceAddressMap &interfaceAddresses)
{
  boost::asio::ip::udp::endpoint multicastEndpoint(serviceConfiguration.getGroupAddress(),
    serviceConfiguration.getPort());
  for (const auto &forwardingRule: serviceConfiguration.getForwardingRules())
  {
    // Figure out source interface address (iterator)
    auto sourceIter = getInterfaceAddress(interfaceAddresses, forwardingRule.getFromInterface(), "joining receiver");

    // Figure out destination interface address
    auto destination = getInterfaceAddress(interfaceAddresses, forwardingRule.getToInterface(), "configuring sender")
      ->second.getAddress();

    // Figure out from which addresses we need to forward datagrams
    auto acceptedSourceNetworks = getAcceptedSourceNetworks(forwardingRule, sourceIter, interfaceAddresses);

    if (m_router != nullptr)
    {
      m_router->addRule(multicastEndpoint, sourceIter->second.getAddress(), acceptedSourceNetworks, destination);
    }
  }
}

int Application::test(std::unique_ptr<Configuration> &&configuration)
{
  try
  {
    Application(std::move(configuration)).setupRouter();
  }
  catch (const std::exception &e)
  {
    std::cerr << e.what() << std::endl
      << "Test failed." << std::endl;
    return 1;
  }
  std::cout << "Test succeeded." << std::endl;
  return 0;
}
