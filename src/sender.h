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

#include <algorithm>
#include <queue>

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/udp.hpp>


/** One sender per outgoing interface */
struct Sender
{
  using address_t = boost::asio::ip::address_v4;
  using endpoint_t = boost::asio::ip::udp::endpoint;


  Sender(boost::asio::io_service &ioService, address_t outInterfaceAddress);

  Sender(const Sender &) = delete;
  Sender &operator =(const Sender &) = delete;

  void send(const char *data, std::size_t length, const endpoint_t &multicastEndpoint);


private:

  struct QueueItem
  {
    QueueItem(const char *data, std::size_t length, const endpoint_t &multicastEndpoint);

    QueueItem(const QueueItem &) = delete;
    QueueItem &operator =(const QueueItem &) = delete;

    QueueItem(QueueItem &&) = default;
    QueueItem &operator =(QueueItem &&) = default;


    const char *getData() const noexcept;
    std::size_t getLength() const noexcept;
    const endpoint_t &getMulticastEndpoint() const noexcept;


  private:

    std::unique_ptr<char[]> m_data;
    std::size_t m_length;
    endpoint_t m_multicastEndpoint;
  };

  void beginSend();

  void endSend(const boost::system::error_code &error, std::size_t bytesTransferred);

  boost::asio::ip::udp::socket m_socket;
  std::queue<QueueItem> m_queue;
};


inline
Sender::QueueItem::QueueItem(const char *data, size_t length, const endpoint_t &multicastEndpoint):
  m_data(std::make_unique<char[]>(length)),
  m_length(length),
  m_multicastEndpoint(multicastEndpoint)
{
  std::copy_n(data, length, m_data.get());
}

inline
const char *Sender::QueueItem::getData() const noexcept
{
  return m_data.get();
}

inline
std::size_t Sender::QueueItem::getLength() const noexcept
{
  return m_length;
}

inline
auto Sender::QueueItem::getMulticastEndpoint() const noexcept -> const endpoint_t &
{
  return m_multicastEndpoint;
}
