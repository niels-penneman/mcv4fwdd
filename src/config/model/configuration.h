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

#include "config/model/serviceconfiguration.h"


namespace config::model
{
  struct Configuration;
}

std::ostream &operator <<(std::ostream &os, const config::model::Configuration &configuration);


struct config::model::Configuration
{
  using service_configurations_t = std::list<ServiceConfiguration>;


  Configuration() = default;

  Configuration(const Configuration &) = delete;
  Configuration &operator =(const Configuration &) = delete;


  void addServiceConfiguration(ServiceConfiguration &&serviceConfiguration);

  service_configurations_t &getServiceConfigurations() noexcept;

  const service_configurations_t &getServiceConfigurations() const noexcept;


private:

  service_configurations_t m_services;
};


inline
void config::model::Configuration::addServiceConfiguration(ServiceConfiguration &&serviceConfiguration)
{
  m_services.emplace_back(std::move(serviceConfiguration));
}

inline
auto config::model::Configuration::getServiceConfigurations() noexcept -> service_configurations_t &
{
  return m_services;
}

inline
auto config::model::Configuration::getServiceConfigurations() const noexcept -> const service_configurations_t &
{
  return m_services;
}
