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


#include "utility.h"

#include <array>
#include <cassert>
#include <cstring>


std::string utility::getErrorString(int error)
{
  std::array<char, 256> buffer;
#if defined(_GNU_SOURCE)
  const char *message = strerror_r(error, buffer.data(), buffer.size());
  if (message != nullptr)
  {
    return message;
  }
#elif (_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600)
  if (strerror_r(error, buffer.data(), buffer.size()) == 0)
  {
    return buffer.data();
  }
#else
#error "Unsupported"
#endif
  int written = snprintf(buffer.data(), buffer.size(), "Unknown error %d", error);
  assert(written > 0 && static_cast<size_t>(written) < buffer.size());
  return buffer.data();
}
