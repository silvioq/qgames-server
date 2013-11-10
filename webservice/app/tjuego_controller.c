/****************************************************************************
 * Copyright (c) 2009-2013 Silvio Quadri                                    *
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
#include  <stdlib.h>
#include  "mongoose.h"
#include  "users.h"
#include  "game_types.h"
#include  "games.h"
#include  "log.h"
#include  "session.h"
#include  "base64.h"
#include  "cJSON.h"
#include  "webserver.h"



/*
 * En esta accion voy a listar los tipos de juego conocido y voy a
 * tratar de brindar toda la informacion disponible acerca de los
 * mismos
 * tjuego puede ser nulo o tener un tipo de juego especifico.
 * En el caso de que sea nulo, solo mostrara cosas basicas
 * como color y piezas.
 * */
static void  game_controller_tjuegos( struct mg_connection* conn, const struct mg_request_info* ri, Session* s, char* tjuego, int format ){
    void* cursor = NULL;
    GameType * gt;
    cJSON*  tjuegos = cJSON_CreateObject();
    while( game_type_next( &cursor, &gt ) ){
        if( tjuego && strcmp( gt->nombre, tjuego ) != 0 ) continue;
        cJSON* json_j = cJSON_CreateObject();
        cJSON* colores = cJSON_CreateArray();
        cJSON* tpiezas = cJSON_CreateArray();

        Tipojuego* tj = game_type_tipojuego( gt );
        if( !tj ){
            LOGPRINT( 2, "Error tratando de obtener tipojuego %s", gt->nombre );
            continue;
        }
        int i = 1; const char* color ;
        while( color = qg_tipojuego_info_color( tj, i ) ){
            cJSON* col = cJSON_CreateObject();
            cJSON_AddStringToObject( col, "nombre", color );
            if( qg_tipojuego_info_color_rotado( tj, i ) ) {
                cJSON_AddTrueToObject( col, "rotado" );
            }
            i ++;
            cJSON_AddItemToArray( colores, col );
        }
        i = 1; const char* tpieza;
        while( tpieza = qg_tipojuego_info_tpieza( tj, i ) ){
            cJSON_AddItemToArray( tpiezas, cJSON_CreateString( tpieza ) );
            i ++;
        }

        cJSON_AddItemToObject( json_j, "colores", colores );
        cJSON_AddItemToObject( json_j, "tipos_pieza", tpiezas );

        // Si no se pidio el tipo de juego, muestro solo info basica
        //
        if( tjuego ){
            cJSON* casilleros, *imagenes;
            casilleros = cJSON_CreateArray();
            i = 1; const char* cas; int* pos;
            int dims = qg_tipojuego_get_dims( tj );
            while( cas = qg_tipojuego_info_casillero( tj, i, &pos ) ){
                cJSON* c = cJSON_CreateObject( );
                cJSON_AddStringToObject( c, "nombre", cas );
                int j;
                for( j = 0; j < dims; j ++ ){
                    char coord[15];
                    sprintf( coord, "coord%d", j );
                    cJSON_AddNumberToObject( c, coord, pos[j] );
                }
                i ++;
                cJSON_AddItemToArray( casilleros, c );
            }
            cJSON_AddItemToObject( json_j, "casilleros", casilleros );

            // vamos por las imagenes
            int w, h;
            void* png;
            int size;
            if( size = qg_tipojuego_get_tablero_png( tj, BOARD_ACTUAL, 0, &png, &w, &h ) ){
                imagenes = cJSON_CreateObject( );

                cJSON* imtabl = cJSON_CreateObject();
                cJSON_AddNumberToObject( imtabl, "height", h );
                cJSON_AddNumberToObject( imtabl, "width", w );
                cJSON_AddBinaryToObject( imtabl, "imagen", png, size );
                qgames_free_png( png );
                cJSON_AddItemToObject( imagenes, "tablero", imtabl );

                if( size = qg_tipojuego_get_logo( tj, &png, &w, &h ) ){
                    cJSON* imlogo = cJSON_CreateObject();
                    cJSON_AddNumberToObject( imlogo, "height", h );
                    cJSON_AddNumberToObject( imlogo, "width", w );
                    cJSON_AddBinaryToObject( imlogo, "imagen", png, size );
                    qgames_free_png( png );
                    cJSON_AddItemToObject( imagenes, "logo", imlogo );
                }


                i = 1;
                cJSON* piezas = cJSON_CreateArray();
                while( tpieza = qg_tipojuego_info_tpieza( tj, i ) ){
                    int c = 1;
                    while( color = qg_tipojuego_info_color( tj, c ) ){
                        if( size = qg_tipojuego_get_tpieza_png( tj, color, tpieza, 0, &png, &w, &h ) ){
                            cJSON* impiez = cJSON_CreateObject();
                            cJSON_AddStringToObject( impiez, "tipo_pieza", tpieza );
                            cJSON_AddStringToObject( impiez, "color", color );
                            cJSON_AddNumberToObject( impiez, "width", w );
                            cJSON_AddNumberToObject( impiez, "height", h );
                            cJSON_AddBinaryToObject( impiez, "imagen", png, size );
                            qgames_free_png( png );
                            cJSON_AddItemToArray( piezas, impiez );
                        }
                        if( size = qg_tipojuego_get_tpieza_png( tj, color, tpieza, GETPNG_PIEZA_CAPTURADA, &png, &w, &h ) ){
                            cJSON* impiez = cJSON_CreateObject();
                            cJSON_AddStringToObject( impiez, "tipo_pieza", tpieza );
                            cJSON_AddStringToObject( impiez, "color", color );
                            cJSON_AddNumberToObject( impiez, "width", w );
                            cJSON_AddNumberToObject( impiez, "height", h );
                            cJSON_AddBinaryToObject( impiez, "imagen", png, size );
                            cJSON_AddTrueToObject( impiez, "capturada" );
                            qgames_free_png( png );
                            cJSON_AddItemToArray( piezas, impiez );
                        }
                        c ++;
                    }
                    i ++;
                }
                cJSON_AddItemToObject( imagenes, "piezas", piezas );
                cJSON_AddItemToObject( json_j, "imagenes", imagenes );
            } else {
                LOGPRINT( 5, "Error al intentar obtener png %s", gt->nombre );
            }
        }
        cJSON_AddItemToObject( tjuegos, gt->nombre, json_j );
    }
    game_type_end( &cursor );
    render_200j( conn, ri, tjuegos, format );
    cJSON_Delete( tjuegos );
}



/*
 * Este es el controlador de game.
 * Lo que voy a hacer es sencillo. 
 * */
void tjuego_controller( struct mg_connection* conn, const struct mg_request_info* ri, Session* s, int action, int format, char* parm ){
    session_save( s ); // toco la sesion
    switch(action){
        case  ACTION_TIPOJUEGOS:
            game_controller_tjuegos( conn, ri, s, parm[0] ? parm : NULL, format );
            break;
        default:
            render_404( conn, ri );
    }

}

/* vi: set cin sw=4: */ 
