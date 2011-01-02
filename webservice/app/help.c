/****************************************************************************
 * Copyright (c) 2009-2011 Silvio Quadri                                    *
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
#include  <string.h>
#include  <stdio.h>
#include  <sys/time.h>
#include  <time.h>
#include  "mongoose.h"
#include  "users.h"
#include  "log.h"
#include  "session.h"
#include  "webserver.h"

/*
 * Este es el controlador de help.
 * */

void help_controller( struct mg_connection* conn, const struct mg_request_info* ri, int action, int format ){
    FILE* f = tmpfile( );
    fprintf( f, "respuesta: OK\ncomandos:\n" );
    fprintf( f, "  \"/login\": Envio de informacion de login (por POST)\n" );
    fprintf( f, "  \"/{sesion}/crea/{tipojuego}\": Crea un nuevo tipo de juego\n" );
    fprintf( f, "  \"/{sesion}/lista\": Lista los juegos activos\n" );
    fprintf( f, "  \"/{sesion}/tablero/{idjuego}\": Detalle del tablero\n" );
    fprintf( f, "  \"/{sesion}/posibles/{idjuego}\": Detalle del tablero y movidas posibles\n" );
    fprintf( f, "  \"/{sesion}/mueve/{idjuego}\": Detalle del tablero (por POST)\n" );
    render_200f( conn, ri, f );
    close( f );
    return;
}
