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
#include <map>

#include <boost/asio/io_service.hpp>

#include "forwarder.h"
#include "sender.h"
#include "config/model/network.h"


struct Router
{
  using address_t = boost::asio::ip::address_v4;
  using endpoint_t = boost::asio::ip::udp::endpoint;


  Router(boost::asio::io_service &ioService);

  Router(const Router &) = delete;
  Router &operator =(const Router &) = delete;

  void addRule(const endpoint_t &multicastEndpoint, address_t fromInterfaceAddress,
    const std::list<config::model::Network> &fromInterfaceAcceptedNetworks, address_t toInterfaceAddress);

  void start();


private:

  boost::asio::io_service &m_ioService;

  std::map<endpoint_t, std::unique_ptr<Forwarder>> m_forwarders;
  std::map<address_t, std::shared_ptr<Sender>> m_senders;
};


inline
Router::Router(boost::asio::io_service &ioService):
  m_ioService(ioService),
  m_forwarders(),
  m_senders()
{}
