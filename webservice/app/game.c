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
#include  <string.h>
#include  <stdio.h>
#include  <sys/time.h>
#include  <time.h>
#include  "mongoose.h"
#include  "users.h"
#include  "games.h"
#include  "log.h"
#include  "session.h"
#include  "webserver.h"


/*
 * Dado un juego, escribe en el archivo pasado como parametro toda la informacion
 * relevante 
 * */
static void  print_game_data( Game* g, Partida* p, FILE* f ){

    int i, movidas ;
    char* res;
    fprintf( f, "game_id: %s\n", g->id );
    fprintf( f, "tipo_juego: %s\n", game_game_type( g )->nombre );
    fprintf( f, "color: %s\n", qg_partida_color( p ) );
    movidas = qg_partida_movhist_count( p );
    fprintf( f, "cantidad_movidas: %d\n", movidas );
    qg_partida_final( p, &res );
    fprintf( f, "descripcion_estado: %s\n", res );
    fprintf( f, "es_continuacion: %s\n", qg_partida_es_continuacion( p ) ? "true" : "false" );
    i = 0;
    if( movidas > 0 ){
        while( res = (char*) qg_partida_movhist_destino( p, movidas - 1, i ) ){
            if( i )
                fprintf( f, ",%s", res );
            else
                fprintf( f, "ultimos_destino: %s" );
            i ++;
        }
        if( i ) fprintf( f, "\n" );
    }
}





static void  game_controller_crea( struct mg_connection* conn, const struct mg_request_info* ri, Session* s, char* game_type ){

    // Obtengo el tipo de juego
    GameType* gt = game_type_share_by_name( game_type );
    Game*     g ;
    if( !gt ){
        render_404( conn, ri );
        return;
    }

    g = game_type_create( gt, session_user( s ) );
    if( !g ){
        render_500( conn, ri, "Error al crear juego" );
        return;
    }
    if( !game_save( g ) ){
        render_500( conn, ri, "Error al salvar juego" );
        return;
    }
    dbact_sync();

    char buff[1024];
    sprintf( buff, "respuesta: OK\ngame_id: %s\nsesion: %.32s\n", 
              g->id, s->id );
    render_200( conn, ri, buff );
    
}

static void  game_controller_tablero( struct mg_connection* conn, const struct mg_request_info* ri, Session* s, char* id ){
    Game*  g = game_load( id );
    if( !g ){
        render_404( conn, ri );
        return;
    }
    Partida* p = game_partida( g );
    int i;
    FILE* f = tmpfile( );
    print_game_data( g, p, f );
    
    int pie = qg_partida_tablero_count( p );
    fprintf( f, "total: %d\n", pie );
    for( i = 0; i < pie; i ++ ){
        char* casillero; char* tipo; char* color;
        qg_partida_tablero_data( p, i, &casillero, &tipo, &color );
        fprintf( f, "- pieza:\n" );
        fprintf( f, "  casillero: %s\n", casillero );
        fprintf( f, "  tipo: %s\n", tipo );
        fprintf( f, "  color: %s\n", color );
    }
    render_200f( conn, ri, f );
    fclose( f );

}


static void  game_controller_posibles( struct mg_connection* conn, const struct mg_request_info* ri, Session* s, char* id ){
    Game*  g = game_load( id );
    if( !g ){
        render_404( conn, ri );
        return;
    }
    Partida* p = game_partida( g );
    int i;
    FILE* f = tmpfile( );
    print_game_data( g, p, f );
    
    int pie = qg_partida_movidas_count( p );
    fprintf( f, "total: %d\nmovidas:\n", pie );
    for( i = 0; i < pie; i ++ ){
        char* notacion; 
        int cap = 0;
        char* pieza, * casillero, * color, * destino;
        char* ttipo, * tcolor;
        qg_partida_movidas_data( p, i, &notacion );
        fprintf( f, "- numero: %d\n  notacion: %s\n", i, notacion );

        if( qg_partida_movidas_pieza( p, i, &casillero, &pieza, &color, &destino, &ttipo, &tcolor ) ){
            fprintf( f, "  pieza: %s\n  color: %s\n  origen: %s\n  destino: %s\n",
                      pieza, color, 
                      casillero == CASILLERO_POZO ? ":pozo" : casillero,
                      destino );
            if( ttipo ){
                fprintf( f, "  transforma_tipo: %s\n  transforma_color: %s\n", ttipo, tcolor );
            }
        }

        while( qg_partida_movidas_capturas( p, i, cap, &casillero, &pieza, &color ) ){
            if( cap == 0 ) fprintf(f, "  es_captura: 1\n  captura:\n" );
            fprintf( f, "  - pieza: %s\n    casillero: %s\n    color: %s\n", pieza, casillero, color );
            cap ++;
        }

        cap = 0;
        while( qg_partida_movidas_crea( p, i, cap, &casillero, &pieza, &color ) ){
            if( cap == 0 ) fprintf( f, "  crea_piezas: \n" );
            fprintf( f, "  - pieza: %s\n    casillero: %s\n    color: %s\n", pieza, casillero, color );
            cap ++;
        }
    }
    render_200f( conn, ri, f );
    fclose( f );

}




/*
 * Este es el controlador de game.
 * Lo que voy a hacer es sencillo. 
 * */

void game_controller( struct mg_connection* conn, const struct mg_request_info* ri, Session* s, int action, char* parm ){
    session_save( s ); // toco la sesion
    switch(action){
        case  ACTION_CREA:
            game_controller_crea( conn, ri, s, parm );
            break;
        case  ACTION_TABLERO:
            game_controller_tablero( conn, ri, s, parm );
            break;
        case  ACTION_POSIBLES:
            game_controller_posibles( conn, ri, s, parm );
            break;
        default:
            render_404( conn, ri );
    }
}
