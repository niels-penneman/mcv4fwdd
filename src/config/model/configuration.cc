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


#include "config/model/configuration.h"

#include <algorithm>
#include <sstream>


using Configuration = config::model::Configuration;


std::ostream &operator <<(std::ostream &os, const Configuration &configuration)
{
  os << "Configuration" << std::endl;
  std::for_each(std::begin(configuration.getServiceConfigurations()), std::end(configuration.getServiceConfigurations()),
    [&](auto &serviceConfiguration) { os << serviceConfiguration; });
  return os;
}


void Configuration::checkInterfaceName(const std::string &interface)
{
  if (interface.length() >= IFNAMSIZ)
  {
    std::ostringstream oss;
    oss << "Interface name exceeds maximum length: " << interface;
    throw std::logic_error(oss.str());
  }
}

std::set<std::string> Configuration::getInterfaces() const
{
  std::set<std::string> interfaces;
  for (const auto &serviceConfiguration: getServiceConfigurations())
  {
    for (const auto &forwardingRule: serviceConfiguration.getForwardingRules())
    {
      checkInterfaceName(forwardingRule.getFromInterface());
      interfaces.insert(forwardingRule.getFromInterface());
      checkInterfaceName(forwardingRule.getToInterface());
      interfaces.insert(forwardingRule.getToInterface());
    }
  }
  return std::move(interfaces);
}
