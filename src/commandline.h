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


#pragma once

#include <string>


struct CommandLine
{
  CommandLine();

  const std::string &getConfigurationFileName() const noexcept;
  bool getForeground() const noexcept;
  const std::string &getPidFileName() const noexcept;
  bool getTestConfigurationOnly() const noexcept;
  int parse(int argc, char *argv[]);
  void printHelp(const char *self, std::ostream &os);


private:

  int doParse(int argc, char *argv[], std::ostream *&helpStream);


  std::string m_configurationFilename;
  std::string m_pidFilename;
  bool m_foreground;
  bool m_testConfigurationOnly;
};

inline
const std::string &CommandLine::getConfigurationFileName() const noexcept
{
  return m_configurationFilename;
}

inline
bool CommandLine::getForeground() const noexcept
{
  return m_foreground;
}

inline
const std::string &CommandLine::getPidFileName() const noexcept
{
  return m_pidFilename;
}

inline
bool CommandLine::getTestConfigurationOnly() const noexcept
{
  return m_testConfigurationOnly;
}
