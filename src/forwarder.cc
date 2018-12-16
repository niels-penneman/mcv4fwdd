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


#include "forwarder.h"

#include <iostream>


using Network = config::model::Network;


void Forwarder::add(const Network &network, const std::shared_ptr<Sender> &sender)
{
  m_sourceNetworksToSenders.emplace_back(network, sender);
}

void Forwarder::handlePacket(const endpoint_t &senderEndpoint, const char *data, std::size_t length)
{
#ifndef NDEBUG
  int forwarded = 0;
  Receiver::handlePacket(senderEndpoint, data, length);
#endif

  // Don't bother with any fancy algorithms here -- m_sourceNetworksToSenders is going to be fairly small
  const auto origin = senderEndpoint.address().to_v4();
  for (auto iter = std::begin(m_sourceNetworksToSenders); iter != std::end(m_sourceNetworksToSenders); ++iter)
  {
    if (iter->first.contains(origin))
    {
#ifndef NDEBUG
      ++forwarded;
#endif
      iter->second->send(data, length, getMulticastEndpoint());
    }
  }
#ifndef NDEBUG
  if (forwarded)
  {
    std::cout << "Datagram queued for forwarding " << forwarded << " times" << std::endl;
  }
  else
  {
    std::cout << "Datagram discarded" << std::endl;
  }
#endif
}

void Forwarder::start()
{
  Receiver::start();
}

std::ostream &operator <<(std::ostream &os, const Forwarder &forwarder)
{
  os << "Forwarder for " << forwarder.getMulticastEndpoint() << "; rules:" << std::endl;
  if (std::empty(forwarder.m_sourceNetworksToSenders))
  {
    os << "\tNone" << std::endl;
  }
  else
  {
    for (auto rule: forwarder.m_sourceNetworksToSenders)
    {
      os << "\t" << rule.first << " -> " << rule.second.get() << std::endl;
    }
  }
  return os;
}
