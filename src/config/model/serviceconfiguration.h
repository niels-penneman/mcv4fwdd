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

#include <list>

#include "config/model/forwardingrule.h"


namespace config::model
{
  struct ServiceConfiguration;
}

std::ostream &operator <<(std::ostream &os, const config::model::ServiceConfiguration &serviceConfiguration);


struct config::model::ServiceConfiguration
{
  using address_t = boost::asio::ip::address_v4;
  using forwarding_rules_t = std::list<ForwardingRule>;

  explicit ServiceConfiguration(const std::string &name);

  ServiceConfiguration(address_t groupAddress, uint16_t port);

  ServiceConfiguration(const ServiceConfiguration &) = delete;
  ServiceConfiguration &operator =(const ServiceConfiguration &) = delete;

  ServiceConfiguration(ServiceConfiguration &&) = default;
  ServiceConfiguration &operator =(ServiceConfiguration &&) = default;


  void addForwardingRule(ForwardingRule &&forwardingRule);

  forwarding_rules_t &getForwardingRules() noexcept;

  const forwarding_rules_t &getForwardingRules() const noexcept;

  address_t getGroupAddress() const noexcept;

  uint16_t getPort() const noexcept;


private:

  address_t m_groupAddress;
  uint16_t m_port;
  forwarding_rules_t m_forwardingRules;
};


inline
void config::model::ServiceConfiguration::addForwardingRule(ForwardingRule &&forwardingRule)
{
  m_forwardingRules.emplace_back(std::move(forwardingRule));
}

inline
auto config::model::ServiceConfiguration::getForwardingRules() noexcept -> forwarding_rules_t &
{
  return m_forwardingRules;
}

inline
auto config::model::ServiceConfiguration::getForwardingRules() const noexcept -> const forwarding_rules_t &
{
  return m_forwardingRules;
}

inline
auto config::model::ServiceConfiguration::getGroupAddress() const noexcept -> address_t
{
  return m_groupAddress;
}

inline
uint16_t config::model::ServiceConfiguration::getPort() const noexcept
{
  return m_port;
}
