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


%option 8bit bison-bridge bison-locations nodefault noyywrap reentrant yylineno warn

%{

#include "config/parser/context.h"

#include <iostream>
#include <memory>

#include "parser.gen.hh"
#include "config/parser/parser.h"
#include "config/parser/tokenvalue.h"


#define YY_EXTRA_TYPE   config::parser::Context *
#define YY_USER_ACTION  yylloc->first_line = yylineno;

#define YY_INPUT(buffer, result, maxSize)                                                                             \
  {                                                                                                                   \
    result = fread(buffer, 1, maxSize, yyextra->getFile());                                                           \
    auto readError = result < maxSize ? ferror(yyextra->getFile()) : 0;                                               \
    if (readError)                                                                                                    \
    {                                                                                                                 \
      yyextra->setReadError(readError);                                                                               \
    }                                                                                                                 \
  }

%}

BYTE                    [0-9]|([1-9][0-9])|(1[0-9]{2})|(2[0-4][0-9])|(25[0-5])
PORT                    ([1-9][0-9]{0,3})|([1-5][0-9]{4})|(64[0-9]{3})|(65[0-4][0-9]{2})|(655[0-2][0-9])|(6553[0-5])
IP_ADDRESS              {BYTE}"."{BYTE}"."{BYTE}"."{BYTE}
IP_ADDRESS_PORT         {IP_ADDRESS}":"{PORT}
SUBNET_PREFIX           [0-9]|(1[0-9])|(2[0-9])|(3[0-2])
NETWORK                 {IP_ADDRESS}("/"{SUBNET_PREFIX})?

%%

[[:space:]\n]                 ;
"#"[^\n]*"\n"                 ; /* # line comments */
"{"                           { return T_BLOCK_BEGIN; }
"}"                           { return T_BLOCK_END; }
"forward"                     { return T_KEYWORD_FORWARD; }
"from"                        { return T_KEYWORD_FROM; }
"service"                     { return T_KEYWORD_SERVICE; }
"to"                          { return T_KEYWORD_TO; }
";"                           { return T_SEMICOLON; }
[[:alpha:]][[:alnum:]_]{0,63} { yylval->stringValue = yytext; return T_IDENTIFIER; }
{IP_ADDRESS_PORT}             { yylval->stringValue = yytext; return T_IP_ADDRESS_PORT; }
{NETWORK}                     { yylval->stringValue = yytext; return T_NETWORK; }
<<EOF>>                       { yyterminate(); }
.                             { return T_UNKNOWN; }

%%

void
config::parser::Context::destroyScanner()
{
   yylex_destroy(m_scanner);
}

void
config::parser::Context::initializeScanner()
{
   yylex_init(&m_scanner);
   yyset_extra(this, m_scanner);
}
