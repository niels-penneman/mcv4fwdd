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

#include "config/model/configuration.h"


namespace config::parser
{
   struct Context;
}


struct config::parser::Context
{
  Context(const std::string &fileName, FILE *stream, model::Configuration &configuration);

  Context(const Context &) = delete;
  Context &operator =(const Context &) = delete;

  ~Context();


  template <class...Ts>
  void addForwardingRule(Ts &&... args);

  template <class...Ts>
  void addServiceConfiguration(Ts &&... args);

  template<class... Ts>
  const model::Network &addNetwork(Ts &&... args);

  FILE *getFile() noexcept;

  const std::string &getFileName() const noexcept;

  void *getScanner() noexcept;

  bool getStatus() noexcept;

  void setReadError(int error) noexcept;

  void updateStatus(bool success) noexcept;


private:

  void initializeScanner();

  void destroyScanner();


  void *m_scanner;
  const std::string &m_fileName;
  FILE *m_stream;
  int m_readError;
  bool m_success;

  model::Configuration &m_configuration;
  model::ServiceConfiguration *m_currentServiceConfiguration;
  model::ForwardingRule *m_currentForwardingRule;
};


inline
config::parser::Context::Context(const std::string &fileName, FILE *stream, model::Configuration &configuration):
  m_scanner(nullptr),
  m_fileName(fileName),
  m_stream(stream),
  m_readError(0),
  m_success(true),
  m_configuration(configuration)
{
  initializeScanner();
}

inline
config::parser::Context::~Context()
{
  destroyScanner();
}

template <class...Ts>
inline
void config::parser::Context::addForwardingRule(Ts &&... args)
{
  m_configuration.getServiceConfigurations().back().addForwardingRule(model::ForwardingRule(std::forward<Ts>(args)...));
}

template <class...Ts>
inline
void config::parser::Context::addServiceConfiguration(Ts &&... args)
{
  m_configuration.addServiceConfiguration(model::ServiceConfiguration(std::forward<Ts>(args)...));
}

template<class... Ts>
inline
const config::model::Network &config::parser::Context::addNetwork(Ts &&... args)
{
  return m_configuration.getServiceConfigurations().back().getForwardingRules().back()
    .addNetwork(model::Network(std::forward<Ts>(args)...));
}

inline
FILE *config::parser::Context::getFile() noexcept
{
  return m_stream;
}

inline
const std::string &config::parser::Context::getFileName() const noexcept
{
  return m_fileName;
}

inline
void *config::parser::Context::getScanner() noexcept
{
  return m_scanner;
}

inline
bool config::parser::Context::getStatus() noexcept
{
  return m_readError == 0 && m_success;
}

inline
void config::parser::Context::setReadError(int error) noexcept
{
  m_readError = error;
}

inline
void config::parser::Context::updateStatus(bool success) noexcept
{
  m_success &= success;
}
