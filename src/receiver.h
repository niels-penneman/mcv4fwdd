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

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/udp.hpp>


/** One receiver per multicast endpoint */
struct Receiver
{
  using address_t = boost::asio::ip::address_v4;
  using endpoint_t = boost::asio::ip::udp::endpoint;


  Receiver(boost::asio::io_service &ioService, const endpoint_t &multicastEndpoint);

  Receiver(const Receiver &) = delete;
  Receiver &operator =(const Receiver &) = delete;


  const endpoint_t &getMulticastEndpoint() const noexcept;

  void joinOnInterface(address_t interfaceAddress);

  virtual void start();


protected:

  virtual void handlePacket(const endpoint_t &senderEndpoint, const char *data, std::size_t length);


private:

  enum
  {
    /** IPv4 headers take at least 20 bytes */
    IPV4_MINIMUM_HEADER_SIZE = 20,

    /** UDP headers have a fixed length of 8 bytes */
    UDP_HEADER_SIZE = 8,

    /** The maximum size of IPv4 datagrams is limited by the 16-bit length field in the IPv4 header */
    MAX_IPV4_UDP_DATAGRAM_SIZE = std::numeric_limits<uint16_t>::max() - IPV4_MINIMUM_HEADER_SIZE - UDP_HEADER_SIZE
  };


  void beginReceive();

  void endReceive(const boost::system::error_code &error, std::size_t length);


  boost::asio::ip::udp::socket m_socket;
  endpoint_t m_multicastEndpoint;
  std::array<char, MAX_IPV4_UDP_DATAGRAM_SIZE> m_buffer;
  endpoint_t m_senderEndpoint;
};


inline
auto Receiver::getMulticastEndpoint() const noexcept -> const endpoint_t &
{
  return m_multicastEndpoint;
}
