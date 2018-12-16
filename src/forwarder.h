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

#include "receiver.h"
#include "sender.h"
#include "config/model/network.h"


struct Forwarder;

std::ostream &operator <<(std::ostream &os, const Forwarder &forwarder);


struct Forwarder final: Receiver
{
  using Receiver::Receiver;


  void add(const config::model::Network &network, const std::shared_ptr<Sender> &sender);

  void start() override;


protected:

  void handlePacket(const endpoint_t &senderEndpoint, const char *data, std::size_t length) override;


private:

  friend std::ostream &operator <<(std::ostream &os, const Forwarder &forwarder);

  std::vector<std::pair<config::model::Network, std::shared_ptr<Sender>>> m_sourceNetworksToSenders;
};
