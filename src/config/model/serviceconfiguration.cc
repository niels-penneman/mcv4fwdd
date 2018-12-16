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


#include "config/model/serviceconfiguration.h"

#include <cassert>
#include <cstring>
#include <stdexcept>


namespace
{
  constexpr unsigned long ip(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4)
  {
    return (static_cast<unsigned long>(b1) << 24)
      | (static_cast<unsigned long>(b2) << 16)
      | (static_cast<unsigned long>(b3) << 8)
      | static_cast<unsigned long>(b4);
  }

  struct WellKnownService
  {
    const char *name;
    unsigned long groupAddress;
    uint16_t port;

    bool operator <(const std::string &other) const noexcept;
    bool operator <(const WellKnownService &other) const noexcept;
    bool operator ==(const char *name) const noexcept;
  };

  bool WellKnownService::operator <(const std::string &other) const noexcept
  {
    return strcmp(name, other.c_str()) < 0;
  }

  bool WellKnownService::operator <(const WellKnownService &other) const noexcept
  {
    return operator <(other.name);
  }

  constexpr const WellKnownService WELL_KNOWN_SERVICES[] = {
    { "mdns", ip(224,   0,   0, 251),  5353 },
    { "ssdp", ip(239, 255, 255, 250),  1900 }
  };
}


using ServiceConfiguration = config::model::ServiceConfiguration;


ServiceConfiguration::ServiceConfiguration(boost::asio::ip::address_v4 groupAddress, uint16_t port):
  m_groupAddress(groupAddress),
  m_port(port),
  m_forwardingRules()
{
  if (port == 0)
  {
    throw std::invalid_argument("port cannot be zero");
  }
}

ServiceConfiguration::ServiceConfiguration(const std::string &name):
  m_groupAddress(),
  m_port(),
  m_forwardingRules()
{
  assert(std::is_sorted(std::begin(WELL_KNOWN_SERVICES), std::end(WELL_KNOWN_SERVICES)));
  auto iter = std::lower_bound(std::begin(WELL_KNOWN_SERVICES), std::end(WELL_KNOWN_SERVICES), name);
  if (iter == std::end(WELL_KNOWN_SERVICES) || iter->name != name)
  {
    throw std::invalid_argument("unknown service");
  }
  m_groupAddress = boost::asio::ip::address_v4(iter->groupAddress);
  m_port = iter->port;
}

std::ostream &operator <<(std::ostream &os, const ServiceConfiguration &serviceConfiguration)
{
  os << "Service " << serviceConfiguration.getGroupAddress().to_string() << ':' << serviceConfiguration.getPort() << std::endl;
  std::for_each(std::begin(serviceConfiguration.getForwardingRules()), std::end(serviceConfiguration.getForwardingRules()),
    [&](auto &forwardingRule) { os << '\t' << forwardingRule; });
  return os;
}
