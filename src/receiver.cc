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


#include "receiver.h"

#include <iostream>

#include <boost/asio.hpp>
#include <boost/bind.hpp>


Receiver::Receiver(boost::asio::io_service &ioService, const endpoint_t &multicastEndpoint):
  m_socket(ioService, boost::asio::ip::udp::v4()),
  m_multicastEndpoint(multicastEndpoint)
{
  // Don't get in the way of others listening on the same multicast endpoint
  m_socket.set_option(boost::asio::ip::udp::socket::reuse_address(true));
  m_socket.bind(boost::asio::ip::udp::endpoint(boost::asio::ip::address_v4::any(), multicastEndpoint.port()));
}

void Receiver::beginReceive()
{
  m_socket.async_receive_from(boost::asio::buffer(m_buffer), m_senderEndpoint,
    boost::bind(&Receiver::endReceive, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

void Receiver::endReceive(const boost::system::error_code &error, std::size_t length)
{
  if (error)
  {
    std::ostringstream msg;
    msg << "receive from " << m_multicastEndpoint << " failed: " << error.message();
    throw std::runtime_error(msg.str());
  }

  if (length > 0)
  {
    handlePacket(m_senderEndpoint, m_buffer.data(), length);
  }

  beginReceive();
}

void Receiver::handlePacket(const endpoint_t &senderEndpoint, const char *data, std::size_t length)
{
#ifdef NDEBUG
  (void)senderEndpoint;
  (void)data;
  (void)length;
#else
  std::cout << "Received datagram of " << length << " bytes from " << senderEndpoint << ": " << std::endl
    << std::string(data, length) << std::endl;
#endif
}

void Receiver::joinOnInterface(boost::asio::ip::address_v4 interfaceAddress)
{
  m_socket.set_option(boost::asio::ip::multicast::join_group(m_multicastEndpoint.address().to_v4(), interfaceAddress));
}

void Receiver::start()
{
  // TODO: check if not already started
  beginReceive();
}
