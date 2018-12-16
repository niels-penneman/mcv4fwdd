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

#include <cstdio>
#include <memory>
#include <string>


namespace utility
{
  struct CFileDeleter;

  using UniqueFilePtr = std::unique_ptr<FILE, CFileDeleter>;

  std::string getErrorString(int error);
}


struct utility::CFileDeleter
{
  void
  operator ()(void *file) noexcept
  {
    if (file != nullptr)
    {
      fclose(reinterpret_cast<FILE *>(file));
    }
  }
};
