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

#include <syslog.h>

#include "application.h"
#include "commandline.h"
#include "utility.h"
#include "config/model/configuration.h"
#include "config/parser/parser.h"


namespace
{
  using Configuration = config::model::Configuration;


  /** Performs the double fork trick to run as a daemon */
  int daemonize(const std::string &pidFileName, bool &exit);

  std::unique_ptr<Configuration> loadConfiguration(const std::string &filename);


  int daemonize(const std::string &pidFileName, bool &exit)
  {
    exit = true;

    // Primary fork
    if (pid_t pid = fork())
    {
      if (pid > 0)
      {
        return 0;
      }
      else
      {
        syslog(LOG_ERR, "Primary fork failed: %m");
        return 1;
      }
    }

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
    if (pid_t pid = fork())
    {
      if (pid > 0)
      {
        return 0;
      }
      else
      {
        syslog(LOG_ERR, "Secondary fork failed: %m");
        return 1;
      }
    }

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

  if (commandLine.getTestConfigurationOnly())
  {
    openlog(argv[0], LOG_PID | LOG_PERROR, LOG_USER);
    return Application::test(std::move(configuration));
  }

  openlog(argv[0], LOG_PID | (commandLine.getForeground() ? LOG_PERROR : 0), LOG_USER);

  if (!commandLine.getForeground())
  {
    bool exit = true;
    int r = daemonize(commandLine.getPidFileName(), exit);
    if (exit)
    {
      return r;
    }
  }

  return Application::run(std::move(configuration));
}
