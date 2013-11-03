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

#include  <stdlib.h>
#include  <stdio.h>
#include  <assert.h>
#include  <qgames.h>
#include  <qgames_analyzer.h>

#include  "users.h"
#include  "session.h"
#include  "mongoose.h"
#include  "webserver.h"
#include  "log.h"

#define    unasesion   "1234567890abcdef1234567890abcedf"

int main(int argc, char** argv){

    loglevel = 5;
    assert( get_ruta( "/login" ) );
    assert( route_controller == CONTROLLER_LOGIN );
    assert( route_action    == ACTION_LOGIN );
    assert( route_format    == 0 );
    assert( route_session[0] == 0 );

    assert( get_ruta( "/" unasesion  "/lista" ) );
    assert( route_controller == CONTROLLER_TJUEGO );
    assert( route_action     == ACTION_TIPOJUEGOS ) ;
    assert( strncmp( unasesion, route_session, 32 ) == 0 );

    assert( get_ruta( "/login.html" ) );
    assert( route_controller == CONTROLLER_LOGIN );
    assert( route_action    == ACTION_LOGIN );
    assert( route_format    == FORMAT_HTML );
    assert( route_session[0] == 0 );

    assert( get_ruta( "/" unasesion  "/tablero/hola@com.ar.png" ) );
    assert( route_controller == CONTROLLER_GAME );
    assert( route_action     == ACTION_TABLERO ) ;
    assert( strncmp( unasesion, route_session, 32 ) == 0 );
    assert( strcmp( route_param, "hola@com.ar" ) == 0 );
    assert( route_format    == FORMAT_PNG );

    assert( get_ruta( "/" unasesion  "/partida/hola@com.ar.png" ) );
    assert( route_controller == CONTROLLER_GAME );
    assert( route_action     == ACTION_PARTIDA ) ;
    assert( strncmp( unasesion, route_session, 32 ) == 0 );
    assert( strcmp( route_param, "hola@com.ar" ) == 0 );
    assert( route_format    == FORMAT_PNG );

    assert( get_ruta( "/" unasesion  "/partida/hola@com.ar.text" ) );
    assert( route_controller == CONTROLLER_GAME );
    assert( route_action     == ACTION_PARTIDA ) ;
    assert( strncmp( unasesion, route_session, 32 ) == 0 );
    assert( strcmp( route_param, "hola@com.ar" ) == 0 );
    assert( route_format    == FORMAT_TXT );

    assert( get_ruta( "/" unasesion  "/partida/hola@com.ar" ) );
    assert( route_controller == CONTROLLER_GAME );
    assert( route_action     == ACTION_PARTIDA ) ;
    assert( strncmp( unasesion, route_session, 32 ) == 0 );
    assert( strcmp( route_param, "hola@com.ar" ) == 0 );
    assert( route_format    == FORMAT_QGAME );


    exit( EXIT_SUCCESS );
}

