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


#include "config/model/forwardingrule.h"

#include <iterator>
#include <iostream>


std::ostream &operator <<(std::ostream &os, const config::model::ForwardingRule &forwardingRule)
{
  os << "Forward from '" << forwardingRule.getFromInterface() << "' to '" << forwardingRule.getToInterface() << "'";
  if (!forwardingRule.getNetworks().empty())
  {
    os << " limited to: " ;
    std::for_each(std::begin(forwardingRule.getNetworks()), std::end(forwardingRule.getNetworks()),
      [&](auto &network) { os << network << ' '; });
  }
  os << std::endl;
  return os;
}
