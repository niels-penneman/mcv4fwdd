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

#include <ostream>
#include <list>
#include <string>

#include "config/model/network.h"


namespace config::model
{
  struct ForwardingRule;
}

std::ostream &operator <<(std::ostream &os, const config::model::ForwardingRule &forwardingRule);


struct config::model::ForwardingRule
{
  using networks_t = std::list<Network>;


  ForwardingRule(const std::string &fromInterface, const std::string &toInterface);

  ForwardingRule(const ForwardingRule &) = delete;
  ForwardingRule &operator =(const ForwardingRule &) = delete;

  ForwardingRule(ForwardingRule &&) = default;
  ForwardingRule &operator =(ForwardingRule &&) = default;


  const Network &addNetwork(Network &&network);

  const std::string &getFromInterface() const noexcept;

  const networks_t &getNetworks() const noexcept;

  const std::string &getToInterface() const noexcept;


private:

  std::string m_fromInterface;
  std::string m_toInterface;
  networks_t m_fromNetworks;
};


inline
config::model::ForwardingRule::ForwardingRule(const std::string &fromInterface, const std::string &toInterface):
  m_fromInterface(fromInterface),
  m_toInterface(toInterface),
  m_fromNetworks()
{}

inline
auto config::model::ForwardingRule::addNetwork(Network &&network) -> const Network &
{
  m_fromNetworks.emplace_back(std::move(network));
  return m_fromNetworks.back();
}

inline
const std::string &config::model::ForwardingRule::getFromInterface() const noexcept
{
  return m_fromInterface;
}

inline
auto config::model::ForwardingRule::getNetworks() const noexcept -> const networks_t &
{
  return m_fromNetworks;
}

inline
const std::string &config::model::ForwardingRule::getToInterface() const noexcept
{
  return m_toInterface;
}
