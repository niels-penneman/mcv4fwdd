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


#include "sender.h"

#include <iostream>

#include <boost/asio.hpp>
#include <boost/bind.hpp>


namespace
{
  std::string getOutInterface(int socket)
  {
    in_addr outInterfaceAddress;
    socklen_t length = sizeof(outInterfaceAddress);
    if (getsockopt(socket, IPPROTO_IP, IP_MULTICAST_IF, &outInterfaceAddress, &length) != 0
      || length != sizeof(outInterfaceAddress))
    {
      return std::string();
    }
    return inet_ntoa(outInterfaceAddress);
  }
}


Sender::Sender(boost::asio::io_service &ioService, address_t outInterfaceAddress):
  m_socket(ioService, boost::asio::ip::udp::v4())
{
  // Outgoing multicast packets default to TTL=1, with loopback to the sending host; disable loopback
  m_socket.set_option(boost::asio::ip::multicast::enable_loopback(false));

  // Explicitly set the outgoing interface
  m_socket.set_option(boost::asio::ip::multicast::outbound_interface(outInterfaceAddress));
}

void Sender::beginSend()
{
  assert(!std::empty(m_queue));

  auto &item = m_queue.front();
#ifndef NDEBUG
  std::cout << "Sending datagram of " << item.getLength() << " bytes to " << item.getMulticastEndpoint()
    << " from interface " << getOutInterface(m_socket.native_handle()) << ": " << std::endl
    << std::string(item.getData(), item.getLength()) << std::endl;
#endif
  m_socket.async_send_to(boost::asio::buffer(item.getData(), item.getLength()), item.getMulticastEndpoint(),
    boost::bind(&Sender::endSend, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

void Sender::endSend(const boost::system::error_code &error, std::size_t bytesTransferred)
{
  if (error)
  {
    std::ostringstream msg;
    msg << "send to " << getOutInterface(m_socket.native_handle()) << " failed: " << error.message();
    throw std::runtime_error(msg.str());
  }
  auto bytesRequested = m_queue.front().getLength();
  if (bytesTransferred != bytesRequested)
  {
    std::cerr << "Warning: datagram truncated: only sent " << bytesTransferred << " out of "
      << bytesRequested << " bytes" << std::endl;
  }
  m_queue.pop();
  if (!std::empty(m_queue))
  {
    beginSend();
  }
}

void Sender::send(const char *data, size_t length, const endpoint_t &multicastEndpoint)
{
  bool sendNow = std::empty(m_queue);
  m_queue.emplace(data, length, multicastEndpoint);
  if (sendNow)
  {
    beginSend();
  }
}
