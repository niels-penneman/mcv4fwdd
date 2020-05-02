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


%defines
%define parse.error verbose
%locations
%pure-parser

%parse-param { config::parser::Context *c }
%lex-param   { void *BISON_SCANNER_ARG }

%{

#include "config/parser/parser.h"

#include <iostream>

#include "config/parser/context.h"
#include "config/parser/tokenvalue.h"
#include "parser.gen.hh"
#include "scanner.gen.h"


using ip_address_v4 = boost::asio::ip::address_v4;


namespace
{
  void
  yyerror(YYLTYPE *location, config::parser::Context *context, const char *error)
  {
    std::cerr << context->getFileName() << ':' << location->first_line << ": error: " << error << std::endl;
    context->updateStatus(false);
  }
}


#define BISON_SCANNER_ARG  c->getScanner()

%}


%define api.value.type {struct TokenValue}


%token                T_BLOCK_BEGIN
%token                T_BLOCK_END
%token <stringValue>  T_IDENTIFIER
%token <stringValue>  T_IP_ADDRESS_PORT
%token                T_KEYWORD_FORWARD
%token                T_KEYWORD_FROM
%token                T_KEYWORD_SERVICE
%token                T_KEYWORD_TO
%token                T_SEMICOLON
%token <stringValue>  T_NETWORK
%token                T_UNKNOWN


%type  <stringValue>  InterfaceName
%type  <stringValue>  Service
%type  <stringValue>  ServiceAddressAndPort
%type  <stringValue>  ServiceName
%type  <stringValue>  Network


%start Configuration
%%

Configuration:
  ServiceConfigurations
  ;

ServiceConfigurations:
  ServiceConfiguration
  | ServiceConfigurations ServiceConfiguration
  ;

ServiceConfiguration:
  T_KEYWORD_SERVICE
  Service
  {
    auto colon = $2.find(':');
    if (colon != std::string::npos)
    {
      c->addServiceConfiguration(ip_address_v4::from_string($2.substr(0, colon)), stoi($2.substr(colon + 1)));
    }
    else
    {
      try
      {
        c->addServiceConfiguration($2);
      }
      catch (const std::invalid_argument &)
      {
        std::cerr << c->getFileName() << ':' << yyloc.first_line << ": error: unknown service: " << $2 << std::endl;
        c->updateStatus(false);
      }
    }
  }
  T_BLOCK_BEGIN
    ForwardingRules
  T_BLOCK_END
  ;

Service:
  ServiceName
  | ServiceAddressAndPort
  ;

ServiceName:
  T_IDENTIFIER
  ;

ServiceAddressAndPort:
  T_IP_ADDRESS_PORT
  ;

ForwardingRules:
  ForwardingRule
  | ForwardingRules ForwardingRule
  ;

ForwardingRule:
  T_KEYWORD_FORWARD InterfaceName T_KEYWORD_TO InterfaceName
  {
    c->addForwardingRule($2, $4);
  }
  ForwardingRuleNetworks
  ;

InterfaceName:
  T_IDENTIFIER
  ;

ForwardingRuleNetworks:
  T_SEMICOLON
  | T_BLOCK_BEGIN FromNetworks T_BLOCK_END
  ;

FromNetworks:
  FromNetwork
  | FromNetworks FromNetwork;

FromNetwork:
  T_KEYWORD_FROM Network T_SEMICOLON
  {
    auto slash = $2.find('/');
    uint8_t prefixLength = 32;
    if (slash != std::string::npos)
    {
      prefixLength = stoi($2.substr(slash + 1));
      $2 = $2.substr(0, slash);
    }
    auto address = ip_address_v4::from_string($2);
    auto &network = c->addNetwork(address, prefixLength);
    if (address != network.getAddress())
    {
      std::cerr << c->getFileName() << ':' << yyloc.first_line << ": warning: address masked from "
        << $2 << "/" << prefixLength << " to " << network << std::endl;
    }
  };

Network:
  T_NETWORK;

%%

bool
config::parser::parse(const std::string &fileName, const utility::UniqueFilePtr &file, model::Configuration &configuration)
{
  Context context(fileName, file.get(), configuration);
  return yyparse(&context) == 0 && context.getStatus();
}
