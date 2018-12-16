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


#include "router.h"

#include <iostream>

using Network = config::model::Network;


void Router::addRule(const endpoint_t &multicastEndpoint, address_t fromInterfaceAddress,
  const std::list<Network> &fromInterfaceAcceptedNetworks, address_t toInterfaceAddress)
{
  /* Use one forwarder for each multicast endpoint, as one receiver can join this endpoint on several interfaces (at
   * most IP_MAX_MEMBERSHIPS). */
  auto forwarderIter = m_forwarders.find(multicastEndpoint);
  if (forwarderIter == std::end(m_forwarders))
  {
    forwarderIter = m_forwarders.emplace(multicastEndpoint,
      std::make_unique<Forwarder>(m_ioService, multicastEndpoint)).first;
  }
  assert(forwarderIter != std::end(m_forwarders));
  auto &forwarder = forwarderIter->second;

  // Set up the receiver
  forwarder->joinOnInterface(fromInterfaceAddress);
  // TODO: check IP_MAX_MEMBERSHIPS

  // Use one sender for each outgoing interface
  auto senderIter = m_senders.find(toInterfaceAddress);
  if (senderIter == std::end(m_senders))
  {
    senderIter = m_senders.emplace(toInterfaceAddress, std::make_shared<Sender>(m_ioService, toInterfaceAddress)).first;
  }
  assert(senderIter != std::end(m_senders));
  auto &sender = senderIter->second;

  // Set up the forwarding
  for (auto &fromAcceptedNetwork: fromInterfaceAcceptedNetworks)
  {
    forwarder->add(fromAcceptedNetwork, sender);
  }
}

void Router::start()
{
#ifndef NDEBUG
  std::cout << "Router configuration:" << std::endl;
#endif
  for (auto &forwarder: m_forwarders)
  {
    forwarder.second->start();
#ifndef NDEBUG
    std::cout << *forwarder.second << std::endl;
#endif
  }
#ifndef NDEBUG
  for (auto &sender: m_senders)
  {
    std::cout << "Sender on " << sender.first << " = " << sender.second.get() << std::endl;
  }
  std::cout << "End of router configuration" << std::endl;
#endif
}
