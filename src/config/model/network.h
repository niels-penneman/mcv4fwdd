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

#include <ostream>

#include <boost/asio/ip/address_v4.hpp>


namespace config::model
{
  /** IPv4 network like boost::asio::ip::network_v4 for boost >= 1.66.0 */
  struct Network;
}

std::ostream &operator <<(std::ostream &os, const config::model::Network &network);


struct config::model::Network
{
  using address_t = boost::asio::ip::address_v4;

  Network() noexcept = default;
  Network(address_t address, uint8_t prefixLength, bool applyMask = true) noexcept;

  Network(const Network &) = default;
  Network &operator =(const Network &) = default;


  bool operator <(const address_t &other) const noexcept;
  bool operator <(const Network &other) const noexcept;

  bool contains(address_t address) const noexcept;
  address_t getAddress() const noexcept;
  address_t getMaskedAddress() const noexcept;
  Network getMaskedNetwork() const noexcept;
  static address_t getMaskedAddress(address_t address, uint8_t prefixLength) noexcept;
  uint8_t getPrefixLength() const noexcept;


private:

  address_t m_address;
  uint8_t m_prefixLength;
};


inline
config::model::Network::Network(address_t address, uint8_t prefixLength, bool applyMask) noexcept:
  m_address(applyMask ? getMaskedAddress(address, prefixLength) : address),
  m_prefixLength(prefixLength)
{}

inline
bool config::model::Network::operator <(const address_t &other) const noexcept
{
  return m_address < other;
}

inline
bool config::model::Network::operator <(const Network &other) const noexcept
{
  return m_address < other.m_address;
}

inline
bool config::model::Network::contains(address_t address) const noexcept
{
  return getMaskedAddress(address, m_prefixLength) == getMaskedAddress();
}

inline
auto config::model::Network::getAddress() const noexcept -> address_t
{
  return m_address;
}

inline
auto config::model::Network::getMaskedAddress() const noexcept -> address_t
{
  return getMaskedAddress(m_address, m_prefixLength);
}

inline
auto config::model::Network::getMaskedAddress(address_t address, uint8_t prefixLength) noexcept -> address_t
{
  return address_t(address.to_ulong() & ~((1UL << (32 - prefixLength)) - 1UL));
}

inline
auto config::model::Network::getMaskedNetwork() const noexcept -> Network
{
  return Network(getMaskedAddress(), m_prefixLength);
}

inline
uint8_t config::model::Network::getPrefixLength() const noexcept
{
  return m_prefixLength;
}


inline
std::ostream &operator <<(std::ostream &os, const config::model::Network &network)
{
  os << network.getAddress().to_string() << '/' << static_cast<unsigned>(network.getPrefixLength());
  return os;
}
