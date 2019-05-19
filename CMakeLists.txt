#  mcv4fwdd: IPv4 Multicast Forwarding Daemon
#  Copyright (C) 2018  Niels Penneman
#
#  This file is part of mcv4fwdd.
#
#  mcv4fwdd is free software: you can redistribute it and/or modify it under the
#  terms of the GNU Affero General Public License as published by the Free
#  Software Foundation, either version 3 of the License, or (at your option) any
#  later version.
#
#  mcv4fwdd is distributed in the hope that it will be useful, but WITHOUT ANY
#  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
#  A PARTICULAR PURPOSE. See the GNU Affero General Public License for more
#  details.
#
#  You should have received a copy of the GNU Affero General Public License
#  along with mcv4fwdd. If not, see <https://www.gnu.org/licenses/>.


cmake_minimum_required (VERSION 2.8.9)
cmake_policy (SET CMP0018 NEW)

project (mcv4fwdd C CXX)


set (SRC_DIR "${PROJECT_SOURCE_DIR}/src")

set (Boost_USE_STATIC_LIBS OFF)
set (Boost_USE_MULTITHREADED ON)
set (Boost_USE_STATIC_RUNTIME OFF)
find_package (Boost 1.62 COMPONENTS system REQUIRED)

find_package (BISON REQUIRED)
find_package (FLEX REQUIRED)


include_directories (
  ${Boost_INCLUDE_DIR}
  ${SRC_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
)

set (default_build_type "Release")
if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message (STATUS "Build type set to default '${default_build_type}'")
  set (CMAKE_BUILD_TYPE "${default_build_type}" CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")
endif()

set (CMAKE_C_FLAGS "-std=c99 -pthread")
set (CMAKE_CXX_FLAGS "-std=c++17 -Werror=return-type -pthread")
set (CMAKE_C_FLAGS_DEBUG "-ggdb3 -fsanitize=address,undefined")
set (CMAKE_C_FLAGS_RELEASE "-DNDEBUG -O2")
set (CMAKE_CXX_FLAGS_DEBUG "-ggdb3 -fsanitize=address,undefined")
set (CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -O2")

if ("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang")
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-covered-switch-default -Wno-exit-time-destructors -Wno-global-constructors -Wno-padded -Wno-pedantic -Wno-unused-macros")
elseif ("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU")
  set (CMAKE_AR "gcc-ar")
  set (CMAKE_RANLIB "gcc-ranlib")
  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wuseless-cast")
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wuseless-cast")
  set (CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -flto -ffunction-sections -fdata-sections")
  set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -flto -ffunction-sections -fdata-sections")
  set (CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -Wl,--gc-sections")
endif ()


bison_target (parser_bison
  ${SRC_DIR}/config/parser/parser.y
  ${CMAKE_CURRENT_BINARY_DIR}/parser.gen.cc
)
flex_target (parser_flex
  ${SRC_DIR}/config/parser/scanner.l
  ${CMAKE_CURRENT_BINARY_DIR}/scanner.gen.cc
  COMPILE_FLAGS "--header-file=${CMAKE_CURRENT_BINARY_DIR}/scanner.gen.h"
)
add_flex_bison_dependency (parser_flex parser_bison)
add_library (parser STATIC
  ${BISON_parser_bison_OUTPUTS}
  ${FLEX_parser_flex_OUTPUTS}
)
set_target_properties (parser
  PROPERTIES COMPILE_FLAGS -w
)

add_executable (mcv4fwdd
  ${SRC_DIR}/application.cc
  ${SRC_DIR}/commandline.cc
  ${SRC_DIR}/forwarder.cc
  ${SRC_DIR}/mcv4fwdd.cc
  ${SRC_DIR}/mcv4fwdd.service
  ${SRC_DIR}/receiver.cc
  ${SRC_DIR}/router.cc
  ${SRC_DIR}/sender.cc
  ${SRC_DIR}/utility.cc
  ${SRC_DIR}/config/model/configuration.cc
  ${SRC_DIR}/config/model/forwardingrule.cc
  ${SRC_DIR}/config/model/serviceconfiguration.cc
)
target_link_libraries (mcv4fwdd
  parser
  ${Boost_LIBRARIES}
)


install (TARGETS mcv4fwdd DESTINATION sbin)
install (FILES ${SRC_DIR}/mcv4fwdd.service DESTINATION /lib/systemd/system)


set (CPACK_GENERATOR "DEB")
set (CPACK_DEBIAN_PACKAGE_MAINTAINER "Niels Penneman")
set (CPACK_PACKAGE_VERSION_MAJOR 0)
set (CPACK_PACKAGE_VERSION_MINOR 9)
set (CPACK_PACKAGE_VERSION_PATCH 2)
set (CPACK_DEBIAN_PACKAGE_RELEASE 1)

include (CPack)
