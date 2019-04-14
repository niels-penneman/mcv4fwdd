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


#include "commandline.h"

#include <iostream>

#include <unistd.h>


namespace
{
  constexpr const char CONFIGURATION_FILE[] = "/etc/mcv4fwdd.conf";
  constexpr const char PID_FILE[] = "/var/run/mcv4fwdd.pid";
}


CommandLine::CommandLine():
  m_configurationFilename(CONFIGURATION_FILE),
  m_pidFilename(PID_FILE),
  m_foreground(false),
  m_testConfigurationOnly(false)
{}

int CommandLine::doParse(int argc, char *argv[], std::ostream *&helpStream)
{
  int option;
  while ((option = getopt(argc, argv, "c:fhnp:")) != -1)
  {
    switch (option)
    {
      case 'c':
        m_configurationFilename = optarg;
        break;
      case 'f':
        m_foreground = true;
        break;
      case 'h':
        helpStream = &std::cout;
        return 0;
      case 'n':
        m_testConfigurationOnly = true;
        break;
      case 'p':
        if (optarg[0] != '/')
        {
          // PID file is created after forking and chdir("/")
          std::cerr << argv[0] << ": PID file path must be absolute" << std::endl;
          return 1;
        }
        m_pidFilename = optarg;
        break;
      default:
        helpStream = &std::cerr;
        return 1;
    }
  }

  if (optind < argc)
  {
    helpStream = &std::cerr;
    *helpStream << argv[0] << ": invalid argument -- '" << argv[optind] << "'" << std::endl;
    return 1;
  }
  return 0;
}

int CommandLine::parse(int argc, char *argv[])
{
  std::ostream *helpStream = nullptr;
  int result = doParse(argc, argv, helpStream);
  if (helpStream)
  {
    printHelp(argv[0], *helpStream);
  }
  return result;
}

void CommandLine::printHelp(const char *self, std::ostream &os)
{
  os << std::endl
     << "mcv4fwdd: IPv4 Multicast Forwarding Daemon" << std::endl
     << "Copyright (C) 2018  Niels Penneman" << std::endl
     << std::endl
     << "Usage: " << self << " [-c CONFIGURATION_FILE] [-f] [-n] [-p PID_FILE]" << std::endl
     << "       " << self << " -h" << std::endl
     << "  -c CONFIGURATION_FILE  Specify path to configuration filename (default: " << CONFIGURATION_FILE << ")"
     << std::endl
     << "  -f                     Run in foreground; do not fork" << std::endl
     << "  -h                     Print this help message" << std::endl
     << "  -n                     Exit after testing configuration" << std::endl
     << "  -p PID_FILE            Specify path to PID filename (default: " << PID_FILE << ")"
     << std::endl
     << std::endl;
}
