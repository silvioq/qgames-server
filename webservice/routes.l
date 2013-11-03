%{
/****************************************************************************
 * Copyright (c) 2009-2010 Silvio Quadri                                    *
 *                                                                          *
 * Permission is hereby granted, free of charge, to any person obtaining a  *
 * copy of this software and associated documentation files (the            *
 * "Software"), to deal in the Software without restriction, including      *
 * without limitation the rights to use, copy, modify, merge, publish,      *
 * distribute, distribute with modifications, sublicense, and/or sell       *
 * copies of the Software, and to permit persons to whom the Software is    *
 * furnished to do so, subject to the following conditions:                 *
 *                                                                          *
 * The above copyright notice and this permission notice shall be included  *
 * in all copies or substantial portions of the Software.                   *
 *                                                                          *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS  *
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF               *
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.   *
 * IN NO EVENT SHALL THE ABOVE COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,   *
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR    *
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR    *
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.                               *
 ****************************************************************************/

#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>
#include  <unistd.h>

#include  <config.h>

#include  "users.h"
#include  "session.h"
#include  "mongoose.h"
#include  "webserver.h"
#include  "log.h"

int    route_controller;
int    route_action;
int    route_format;

char   route_session[32];
char   route_param[100];

%}

%option noyywrap
%option fast
%option batch
%option case-insensitive
%option noyylineno

%x      check_gameid    format  with_session  check_param

SESION  [0-9a-fA-F]{32}
GAMEID  [0-9a-zA-Z\@\.]+
WORD    [a-zA-Z\?][a-zA-Z0-9\-\_]{1,99}

TYPE_YAML   ya?ml
TYPE_HTML   html?
TYPE_JSON   json
TYPE_QGAME  qgame
TYPE_PNG    png
TYPE_PGN    pgn
TYPE_XML    xml
TYPE_TXT    te?xt

FORMAT  ({TYPE_YAML})|({TYPE_HTML})|({TYPE_JSON})|({TYPE_QGAME})|({TYPE_PNG})|({TYPE_PGN})|({TYPE_XML})|({TYPE_TXT})

%%

<INITIAL>\/{SESION}      { strncpy( route_session, yytext + 1, 32 ); 
                           route_session[32] = 0;
                           BEGIN( with_session );
                         }

<INITIAL>\/login         { route_controller = CONTROLLER_LOGIN; 
                           route_action     = ACTION_LOGIN;
                           BEGIN(format);
                         }
<INITIAL><<EOF>>         { route_controller = CONTROLLER_LOGIN;
                           route_action     = ACTION_WELCOME;
                           yyterminate( );
                         }
<with_session>\/logout  {  route_controller = CONTROLLER_LOGIN;
                           route_format     = FORMAT_TXT;
                           route_action     = ACTION_LOGOUT;
                           BEGIN(format);
                        }
<INITIAL>\/ping          { route_controller = CONTROLLER_LOGIN;
                           route_format     = FORMAT_TXT;
                           route_action     = ACTION_PING;
                           BEGIN(format);
                         }

<INITIAL>\/help          { route_controller = CONTROLLER_HELP;
                           route_action     = ACTION_INDEX;
                           BEGIN(format);
                         }

<with_session>\/crea\/   { route_controller = CONTROLLER_GAME;
                           route_action     = ACTION_CREA;
                           BEGIN( check_param );
                         }
<with_session>\/historial\/ { 
                           route_controller = CONTROLLER_GAME;
                           route_action     = ACTION_HISTORIAL;
                           BEGIN( check_gameid );
                        }
<with_session>\/lista\/    { route_controller = CONTROLLER_TJUEGO;
                           route_action     = ACTION_TIPOJUEGOS;
                           BEGIN( check_param );
                        }
<with_session>\/lista    { route_controller = CONTROLLER_TJUEGO;
                           route_action     = ACTION_TIPOJUEGOS;
                           BEGIN(format);
                        }
<with_session>\/move\/   { route_controller = CONTROLLER_GAME;
                           route_action     = ACTION_MUEVE;
                           BEGIN( check_gameid );
                         }
<with_session>\/mueve\/  { route_controller = CONTROLLER_GAME;
                           route_action     = ACTION_MUEVE;
                           BEGIN( check_gameid );
                         }

<with_session>\/tablero\/ { route_controller = CONTROLLER_GAME;
                           route_action     = ACTION_TABLERO;
                           BEGIN( check_gameid );
                         }

<with_session>\/posibles\/ { route_controller = CONTROLLER_GAME;
                           route_action     = ACTION_POSIBLES;
                           BEGIN( check_gameid );
                         }

<with_session>\/registra\/ { route_controller = CONTROLLER_GAME;
                           route_action     = ACTION_REGISTRA;
                           BEGIN( check_gameid );
                         }
<with_session>\/registraciones { 
                           route_controller = CONTROLLER_GAME;
                           route_action     = ACTION_REGISTRACIONES;
                         }
<with_session>\/desregistra\/ { route_controller = CONTROLLER_GAME;
                           route_action     = ACTION_DESREGISTRA;
                           BEGIN( check_gameid );
                         }
<with_session>\/partida\/ { route_controller = CONTROLLER_GAME;
                           route_action     = ACTION_PARTIDA;
                           route_format     = FORMAT_QGAME;
                           BEGIN( check_gameid );
                         }




<check_param>{WORD}      { strcpy( route_param, yytext );
                           BEGIN( format );
                         }

<check_gameid>{GAMEID}{FORMAT} { 
                            int i = yyleng - 1; int encontrado = 0;
                            while( i ){
                              i --;
                              if( yytext[i] == '.' ){
                                yyless( i );
                                strncpy( route_param, yytext, i );
                                BEGIN( format );
                                encontrado = 1;
                                break;
                              }
                            }
                            if( !encontrado ) error( "error resolviendo gameid" );
                         }
<check_gameid>{GAMEID}   { strcpy( route_param, yytext );
                         }




<format>\.{TYPE_YAML}   { route_format = FORMAT_YAML; } 
<format>\.{TYPE_HTML}   { route_format = FORMAT_HTML; } 
<format>\.{TYPE_JSON}   { route_format = FORMAT_JSON; } 
<format>\.{TYPE_QGAME}  { route_format = FORMAT_QGAME; } 
<format>\.{TYPE_PNG}    { route_format = FORMAT_PNG; } 
<format>\.{TYPE_PGN}    { route_format = FORMAT_PGN; } 
<format>\.{TYPE_TXT}    { route_format = FORMAT_TXT; } 
<format>\.{TYPE_XML}    { route_format = FORMAT_XML; } 


%%

int    get_ruta( char* uri ){
    route_session[0] = 0;
    route_param[0]   = 0;
    route_controller = 0;
    route_action     = 0;
    route_format     = 0;

    yy_scan_string( uri );
    BEGIN( INITIAL );
    yylex();
    yy_delete_buffer( YY_CURRENT_BUFFER );
    return route_controller;

}

