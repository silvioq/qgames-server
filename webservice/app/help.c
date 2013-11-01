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
#include  "cJSON.h"
#include  "users.h"
#include  "log.h"
#include  "session.h"
#include  "webserver.h"

/*
 * Este es el controlador de help.
 * */

void help_controller( struct mg_connection* conn, const struct mg_request_info* ri, int action, int format ){
    if( format == FORMAT_JSON ){
      cJSON* j = cJSON_CreateObject();
      cJSON* comm = cJSON_CreateObject();
      cJSON_AddStringToObject( j, "respuesta", "OK" );
      cJSON_AddItemToObject( j, "comandos", comm );
      cJSON_AddStringToObject( comm, "/login", "Envio de informacion de login (por POST)" );
      cJSON_AddStringToObject( comm, "/{sesion}/crea/{tipojuego}", "Crea un nuevo juego" );
      cJSON_AddStringToObject( comm, "/{sesion}/historial/{idjuego}", "Historial de movidas" );
      cJSON_AddStringToObject( comm, "/{sesion}/lista", "Lista los tipos de juego activos" );
      cJSON_AddStringToObject( comm, "/{sesion}/lista/{tipojuego}", "Lista detalles de tipo de juego" );
      cJSON_AddStringToObject( comm, "/{sesion}/tablero/{idjuego}", "Detalle del tablero" );
      cJSON_AddStringToObject( comm, "/{sesion}/posibles/{idjuego}", "Detalle del tablero y movidas posibles" );
      cJSON_AddStringToObject( comm, "/{sesion}/mueve/{idjuego}", "Detalle del tablero (por POST)" );
      cJSON_AddStringToObject( comm, "/{sesion}/registraciones", "Partidas registradas actualmente en el servidor" );
      cJSON_AddStringToObject( comm, "/{sesion}/partida/{idjuego}", "Descarga de la partida en formato binario" );
      cJSON_AddStringToObject( comm, "/{sesion}/registra/{idjuego}", "Registra un nuevo juego en el servidor" );
      cJSON_AddStringToObject( comm, "/{sesion}/desregistra/{idjuego}", "Quita un juego en el servidor" );
      char* p = cJSON_Print( j );
      render_200( conn, ri, p );
      free( p );
      cJSON_Delete( j );
    } else {
      FILE* f = tmpfile( );
      fprintf( f, "respuesta: OK\ncomandos:\n" );
      fprintf( f, "  \"/login\": Envio de informacion de login (por POST)\n" );
      fprintf( f, "  \"/{sesion}/crea/{tipojuego}\": Crea un nuevo juego\n" );
      fprintf( f, "  \"/{sesion}/historial/{idjuego}\": Historial de movidas\n" );
      fprintf( f, "  \"/{sesion}/lista\": Lista los tipos de juego activos\n" );
      fprintf( f, "  \"/{sesion}/lista/{tipojuego}\": Lista detalles de tipo de juego\n" );
      fprintf( f, "  \"/{sesion}/tablero/{idjuego}\": Detalle del tablero\n" );
      fprintf( f, "  \"/{sesion}/posibles/{idjuego}\": Detalle del tablero y movidas posibles\n" );
      fprintf( f, "  \"/{sesion}/mueve/{idjuego}\": Detalle del tablero (por POST)\n" );
      fprintf( f, "  \"/{sesion}/registraciones\": Partidas registradas actualmente en el servidor\n" );
      fprintf( f, "  \"/{sesion}/partida/{idjuego}\": Descarga de la partida en formato binario\n" );
      fprintf( f, "  \"/{sesion}/registra/{idjuego}\": Registra un nuevo juego en el servidor\n" );
      fprintf( f, "  \"/{sesion}/desregistra/{idjuego}\": Quita un juego en el servidor\n" );
      render_200f( conn, ri, f );
      close( f );
    }
    return;
}
