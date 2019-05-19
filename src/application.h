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

#include <memory>

#include <boost/asio.hpp>

#include "router.h"
#include "config/model/configuration.h"


struct Application final
{
  using Configuration = config::model::Configuration;
  using InterfaceAddressMap = std::multimap<std::string, config::model::Network>;


  Application(const Application &) = delete;
  Application(Application &&) = delete;

  Application &operator =(const Application &) = delete;
  Application &operator =(Application &&) = delete;

  static int run(std::unique_ptr<Configuration> &&configuration);

  static int test(std::unique_ptr<Configuration> &&configuration);


private:

  using deadline_timer = boost::asio::deadline_timer;
  using io_service = boost::asio::io_service;
  using ServiceConfiguration = config::model::ServiceConfiguration;


  Application(std::unique_ptr<Configuration> &&configuration);

  void doRestart(const boost::system::error_code &error);

  int run();

  void scheduleRestart();

  /** Creates and configures the router */
  void setupRouter();

  /** Translates the parsed configuration file into a run-time configuration for the router */
  void setupRouterConfiguration(const InterfaceAddressMap &interfaceAddresses);

  /** Translates the given ServiceConfiguration into a run-time configuration for the router */
  void setupRouterConfiguration(const ServiceConfiguration &ServiceConfiguration,
    const InterfaceAddressMap &interfaceAddresses);


  std::unique_ptr<Configuration> m_configuration;
  std::shared_ptr<io_service> m_ioService;
  std::unique_ptr<deadline_timer> m_resetTimer;
  std::unique_ptr<Router> m_router;
};
