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
#include  "base64.h"
#include  "webserver.h"

#include  <pthread.h>

/*
 * Dado un juego, modfica la partida en el caso que no este previamente
 * calculada
 * */
static  int   save_game_if_not_calculed( Game* g, Partida* p ){
    if( qg_partida_movidas_analizadas( p ) ) return 1;
    char* res;
    qg_partida_movidas_count( p );
    if( qg_partida_movidas_analizadas( p ) ){
        game_set_partida( g, p );
        return game_save( g );
    } else if( qg_partida_final( p, NULL ) != FINAL_ENJUEGO ){
        return 1;
    } else{
        LOGPRINT( 1, "Extrañamente, no se calcularon bien las posiciones %d", 0 );
        return 0;
    }
        
}

static  void  print_game_data_prefix( Game* g, Partida* p, FILE* f, int spaces ){
    char  prefix[80];
    prefix[spaces] = 0;
    if( spaces ){
        while( spaces ){
           prefix[spaces - 1] = ' ';
           spaces --;
        }
    }
    int i, movidas ;
    char* res;
    static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

    pthread_mutex_lock( &mutex );
    fprintf( f, "%sgame_id: %s\n", prefix, g->id );
    fprintf( f, "%stipo_juego: %s\n", prefix, game_game_type( g )->nombre );
    fprintf( f, "%scolor: %s\n", prefix, qg_partida_color( p ) );
    movidas = qg_partida_movhist_count( p );
    fprintf( f, "%scantidad_movidas: %d\n", prefix, movidas );
    qg_partida_final( p, &res );
    fprintf( f, "%sdescripcion_estado: %s\n", prefix, res ? res : "Jugando" );
    fprintf( f, "%ses_continuacion: %s\n", prefix, qg_partida_es_continuacion( p ) ? "true" : "false" );
    i = 0;
    if( movidas > 0 ){
        while( res = (char*) qg_partida_movhist_destino( p, movidas - 1, i ) ){
            if( i )
                fprintf( f, ",%s", res );
            else
                fprintf( f, "%sultimos_destino: %s", prefix, res );
            i ++;
        }
        if( i ) fprintf( f, "\n" );
    }
    pthread_mutex_unlock( &mutex );
}


/*
 * Dado un juego, escribe en el archivo pasado como parametro toda la informacion
 * relevante 
 * */
static void  print_game_data( Game* g, Partida* p, FILE* f ){
    print_game_data_prefix( g, p, f, 0 );
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

    FILE* f = tmpfile( );
    Game* g2 = game_load( g->id );   // FIXME: Es necesario volver a leer. Corregir la funcion game_save para
                                     //        que quede como corresponde
    game_free( g );
    g = g2;    
    Partida* p = game_partida( g );
    print_game_data( g, p, f );
    render_200f( conn, ri, f );
    fclose( f );
    game_free( g );
    
}

static void  game_controller_tablero( struct mg_connection* conn, const struct mg_request_info* ri, Session* s, char* id ){
    Game*  g = game_load( id );
    if( !g ){
        render_404( conn, ri );
        return;
    }
    LOGPRINT( 5, "El usuario es %s", session_user( s )->code );
    if( !game_check_user( g, session_user( s ) ) ){
        render_500( conn, ri, "Usuario no autorizado" );
        game_free( g );
        return;
    }
    Partida* p = game_partida( g );
    if( !save_game_if_not_calculed( g, p ) ){
        render_500( conn, ri, "Error al intentar recalcular partida" );
        return;
    }
    int i;
    FILE* f = tmpfile( );
    print_game_data( g, p, f );
    
    int pie = qg_partida_tablero_count( p, LAST_MOVE );
    int cap = qg_partida_tablero_countcap( p, LAST_MOVE );
    fprintf( f, "total: %d\npiezas:\n", pie );
    for( i = 0; i < pie; i ++ ){
        char* casillero; char* tipo; char* color;
        qg_partida_tablero_data( p, LAST_MOVE, i, &casillero, &tipo, &color );
        fprintf( f, "- casillero: %s\n", casillero );
        fprintf( f, "  tipo: %s\n", tipo );
        fprintf( f, "  color: %s\n", color );
    }
    for( i = 0; i < cap; i ++ ){
        char* tipo; char* color;
        qg_partida_tablero_datacap( p, LAST_MOVE, i, &tipo, &color );
        fprintf( f, "- casillero: :captured\n" );
        fprintf( f, "  tipo: %s\n", tipo );
        fprintf( f, "  color: %s\n", color );
    }
    render_200f( conn, ri, f );
    game_free( g );
    fclose( f );

}

static void  game_controller_historial( struct mg_connection* conn, const struct mg_request_info* ri, Session* s, char* id ){
    Game*  g = game_load( id );
    Movdata movd;
    if( !g ){
        LOGPRINT( 2, "Juego no encontrado (raro) %s", id );
        render_404( conn, ri );
        return;
    }
    if( !game_check_user( g, session_user( s ) ) ){
        render_500( conn, ri, "Usuario no autorizado" );
        game_free( g );
        return;
    }
    Partida* p = game_partida( g );
    if( !save_game_if_not_calculed( g, p ) ){
        render_500( conn, ri, "Error al intentar recalcular partida" );
        return;
    }
    int i;
    FILE* f = tmpfile( );
    print_game_data( g, p, f );
    
    fprintf( f, "tablero_inicial:\n" );
    i = 0;
    while( true ){
        char* casillero; char* tipo; char* color;
        if( !qg_partida_tablero_data( p, 0, i, &casillero, &tipo, &color ) ) break;
        fprintf( f, "- casillero: %s\n", casillero );
        fprintf( f, "  tipo: %s\n", tipo );
        fprintf( f, "  color: %s\n", color );
        i ++;
    }


    i = 0;
    fprintf( f, "movidas:\n" );
    while( qg_partida_movhist_data( p, i, &movd ) ){
        fprintf( f, "- numero: %d\n", movd.numero );
        fprintf( f, "  descripcion: %s\n", movd.descripcion );
        fprintf( f, "  pieza: %s\n", movd.pieza );
        fprintf( f, "  color: %s\n", movd.color );
        if( movd.captura ){
            fprintf( f, "  es_captura: 1\n" );
            fprintf( f, "  captura: %s\n", movd.captura_pieza );
            fprintf( f, "  captura_cas: %s\n", movd.captura_casillero );
        }
        fprintf( f, "  origen: %s\n", ( movd.origen == CASILLERO_POZO ? ":pozo" : movd.origen ) );
        fprintf( f, "  destino: %s\n", movd.destino );
        fprintf( f, "  notacion: %s\n", movd.notacion );
        if( movd.transforma ){
            fprintf( f, "  transforma_tipo: %s\n", movd.transforma_pieza );
            fprintf( f, "  transforma_color: %s\n", movd.transforma_color );
        }
        if( movd.movida ){
            fprintf( f, "  detalle:\n" );
            do{
                fprintf( f, "  - origen: %s\n", ( movd.movida_origen == CASILLERO_POZO ? ":pozo" : movd.movida_origen ) );
                fprintf( f, "  - destino: %s\n", ( movd.movida_destino == CASILLERO_POZO ? ":pozo" : movd.movida_destino ) );
            } while( qg_partida_movdata_nextmov( p, &movd ) );
        }

        if( movd.crea ){
            fprintf( f, "  crea: \n" );
            do {
                fprintf( f, "  - pieza: %s\n    casillero: %s\n    color: %s\n", 
                        movd.crea_pieza, movd.crea_casillero, movd.crea_color );
            } while( qg_partida_movdata_nextcrea( p, &movd ) );
        }
        i ++;

    }

    render_200f( conn, ri, f );
    fclose( f );

}

/*
 * Realiza la movida pasada como parametro
 * */
static void  game_controller_mueve( struct mg_connection* conn, const struct mg_request_info* ri, Session* s, char* id ){
    if( strcmp( ri->request_method, "POST" ) != 0 ){
        render_400( conn, ri, "Debe enviar movida (m=xxxx) por POST" );
        return;
    }
    char* move = mg_get_var( conn, "m" );
    if( !move ){
        render_400( conn, ri, "Debe enviar movida (m=numero de movida)" );
        return;
    }
    Game*  g = game_load( id );
    if( !g ){
        render_404( conn, ri );
        free( move );
        return;
    }
    if( !game_check_user( g, session_user( s ) ) ){
        render_500( conn, ri, "Usuario no autorizado" );
        game_free( g );
        free( move );
        return;
    }
    Partida* p = game_partida( g );
    if( move[0] >= '0' && move[0] <= '9' ){
        int move_number = atoi( move );
        if( !qg_partida_mover( p, move_number ) ){
            render_400( conn, ri, "Movida incorrecta" );
            free( move );
            game_free( g );
            return;
        }
    } else {
        if( !qg_partida_movida_valida( p, move ) ){
            render_400( conn, ri, "Movida invalida" );
            free( move );
            game_free( g );
            return;
        }
        if( !qg_partida_mover_notacion( p, move ) ){
            render_400( conn, ri, "Movida notada incorrecta" );
            free( move );
            game_free( g );
            return;
        }
    }
    FILE* f = tmpfile( );
    game_set_partida( g, p );
    print_game_data( g, p, f );
    if(!game_save( g ) ){
        LOGPRINT( 5, "Error de partida!", 0 );
        render_400( conn, ri, "Error al guardar partida" );
    } else {
        dbact_sync();
        render_200f( conn, ri, f );
    }
    fclose( f );
    free( move );
    game_free( g );
}


/*
 * esta es la accion que devuelve aquellos movimientos posibles
 * a realizar
 * */
static void  game_controller_posibles( struct mg_connection* conn, const struct mg_request_info* ri, Session* s, char* id ){
    Game*  g = game_load( id );
    if( !g ){
        render_404( conn, ri );
        return;
    }
    LOGPRINT( 5, "Mirando posibles de usuario => %d", s->user_id );
    if( !game_check_user( g, session_user( s ) ) ){
        render_500( conn, ri, "Usuario no autorizado" );
        game_free( g );
        return;
    }
    Partida* p = game_partida( g );
    if( !save_game_if_not_calculed( g, p ) ){
        render_500( conn, ri, "Error al intentar recalcular partida" );
        game_free( g );
        return;
    }

    int i;
    FILE* f = tmpfile( );
    print_game_data( g, p, f );
    
    int pie = qg_partida_movidas_count( p );
    fprintf( f, "total: %d\nmovidas:\n", pie );
    for( i = 0; i < pie; i ++ ){
        Movdata movd;
        if( qg_partida_movidas_data( p, i, &movd ) ){
            fprintf( f, "- numero: %d\n  notacion: %s\n  descripcion: %s\n", i, movd.notacion, movd.descripcion );
            fprintf( f, "  pieza: %s\n  color: %s\n  origen: %s\n  destino: %s\n",
                      movd.pieza, movd.color, 
                      movd.origen == CASILLERO_POZO ? ":pozo" : movd.origen,
                      movd.destino );
            if( movd.transforma ){
                fprintf( f, "  transforma_tipo: %s\n  transforma_color: %s\n", 
                        movd.transforma_pieza, movd.transforma_color );
            }
        }

        if( movd.captura ){
            fprintf(f, "  es_captura: 1\n  captura:\n" );
            do {
                fprintf( f, "  - pieza: %s\n    casillero: %s\n    color: %s\n", 
                        movd.captura_pieza, movd.captura_casillero, movd.captura_color );
            } while( qg_partida_movdata_nextcap( p, &movd ) );
        }

        if( movd.crea ){
            fprintf( f, "  crea_piezas: \n" );
            do {
                fprintf( f, "  - pieza: %s\n    casillero: %s\n    color: %s\n", 
                        movd.crea_pieza, movd.crea_casillero, movd.crea_color );
            } while( qg_partida_movdata_nextcrea( p, &movd ) );
        }
    }
    render_200f( conn, ri, f );
    fclose( f );
    game_free( g );

}

/*
 * Esta accion permite registrar un juego a partir de su binario
 * */
static void  game_controller_registra( struct mg_connection* conn, const struct mg_request_info* ri, Session* s, char* id ){
    if( strcmp( ri->request_method, "POST" ) != 0 ){
        render_400( conn, ri, "Debe enviar registracion por POST" );
        return;
    }
    Game*  g = game_load( id );
    if( g ){
       // TODO: Chequear que sea posible modificar el juego, de acuerdo al usuario
       if( !game_check_user( g, session_user( s ) ) ){
           render_500( conn, ri, "Usuario no autorizado" );
           game_free( g );
           return;
       }
       game_free( g );
    }
    char*  game_type = strdup( qg_partida_load_tj( ri->post_data, ri->post_data_len ) );
    LOGPRINT( 5, "recibo por post %d => %s", ri->post_data_len, game_type );
    GameType* gt = game_type_share_by_name( game_type );
    if( !gt ){
        LOGPRINT( 2, "Error al intentar encontrar el tipo de juego %s", game_type );
        free( game_type );
        render_500( conn, ri, "No se puede encontrar el tipo de juego" );
        return;
    }
    free( game_type );

    Partida* p = qg_partida_load( gt->tipojuego, ri->post_data, ri->post_data_len );
    if( !p ){
        render_500( conn, ri, "Error al intentar leer la partida" );
        return;
    }
    g = game_new( id, session_user( s ), gt, 0 );
    if( ! g ){
        render_500( conn, ri, "Error al crear el juego" );
        return;
    }
    game_set_partida( g, p );
    if( ! game_save( g ) ){
        render_500( conn, ri, "Error al guardar el juego" );
        return;
    }

    FILE* f = tmpfile( );
    print_game_data( g, p, f );
    render_200f( conn, ri, f );
    fclose( f );
    

}

/*
 * Dado un juego existente en la base, se desregistra o elimina
 * */
static void  game_controller_desregistra( struct mg_connection* conn, const struct mg_request_info* ri, Session* s, char* id ){
    
    Game*  g = game_load( id );
    if( !g ){
        render_404( conn, ri );
        return;
    }
    if( !game_check_user( g, session_user( s ) ) ){
        render_500( conn, ri, "Usuario no autorizado" );
        game_free( g );
        return;
    }
    game_del( g );
    game_free( g );
    render_200( conn, ri, "Partida desregistrada" );
}

/*
 * Dado un juego existente en la base, devuelve el binario asociado
 * */
static void  game_controller_partida( struct mg_connection* conn, const struct mg_request_info* ri, Session* s, char* id ){
    
    Game*  g = game_load( id );
    if( !g ){
        render_404( conn, ri );
        return;
    }
    if( !game_check_user( g, session_user( s ) ) ){
        render_500( conn, ri, "Usuario no autorizado" );
        game_free( g );
        return;
    }
    int   status = 200;
    char* reason = "OK";
		mg_printf(conn,
		    "HTTP/1.1 %d %s\r\n"
		    "Content-Type: application/qgame\r\n"
		    "Content-Length: %d\r\n"
		    "Connection: close\r\n"
		    "\r\n", status, reason, g->data_size);

    mg_write( conn, g->data, g->data_size );
    game_free( g );

}

/*
 * En esta accion se devuelven los juegos activos del usuario
 * que esta en la sesion
 * */
static void  game_controller_registraciones( struct mg_connection* conn, const struct mg_request_info* ri, Session* s ){
    void* games = NULL;
    int  primera_vez = 1, cantidad = 0;
    Game* g ;
    User* u = session_user( s );
    FILE* f = tmpfile( );
    while( game_next( &games, &g ) ){
        if( game_check_user( g, u ) ){
            if( primera_vez ){
                fprintf( f, "games:\n" );
                primera_vez = 0;
            }
            fprintf( f, "  -\n" );
            print_game_data_prefix( g, game_partida( g ), f, 4 );
            cantidad ++;
        }
        game_free( g );
    }
    fprintf( f, "respuesta: OK\ncantidad: %d\n", cantidad );
    game_type_end( &games );
    render_200f( conn, ri, f );
    close( f );
    
}


/*
 * En esta accion voy a listar los tipos de juego conocido y voy a
 * tratar de brindar toda la informacion disponible acerca de los
 * mismos
 * tjuego puede ser nulo o tener un tipo de juego especifico.
 * En el caso de que sea nulo, solo mostrara cosas basicas
 * como color y piezas.
 * */
static void  game_controller_tjuegos( struct mg_connection* conn, const struct mg_request_info* ri, Session* s, char* tjuego ){
    FILE* f = tmpfile( );
    void* cursor = NULL;
    GameType * gt;
    while( game_type_next( &cursor, &gt ) ){
        if( tjuego && strcmp( gt->nombre, tjuego ) != 0 ) continue;
        fprintf( f, "%s:\n", gt->nombre );
        Tipojuego* tj = game_type_tipojuego( gt );
        if( !tj ){
            LOGPRINT( 2, "Error tratando de obtener tipojuego %s", gt->nombre );
            continue;
        }
        int i = 1; const char* color ;
        fprintf( f, "  colores:\n" );
        while( color = qg_tipojuego_info_color( tj, i ) ){
            fprintf( f, "  - nombre: %s\n", color );
            if( qg_tipojuego_info_color_rotado( tj, i ) ) {
                fprintf( f, "    rotado: true\n" );
            }
            i ++;
        }
        i = 1; const char* tpieza;
        fprintf( f, "  tipos_pieza:\n" );
        while( tpieza = qg_tipojuego_info_tpieza( tj, i ) ){
            fprintf( f, "  - %s\n", tpieza );
            i ++;
        }

        // Si no se pidio el tipo de juego, muestro solo info basica
        //
        if( !tjuego ) continue;

        i = 1; const char* cas; int* pos;
        int dims = qg_tipojuego_get_dims( tj );
        fprintf( f, "  casilleros:\n" );
        while( cas = qg_tipojuego_info_casillero( tj, i, &pos ) ){
            fprintf( f, "  - nombre: %s\n", cas );
            int j;
            for( j = 0; j < dims; j ++ ){
                fprintf( f, "    coord%d: %d\n", j, pos[j] );
            }
            i ++;
        }

        // vamos por las imagenes
        int w, h;
        void* png;
        int size;
        if( size = qg_tipojuego_get_tablero_png( tj, BOARD_ACTUAL, 0, &png, &w, &h ) ){
            fprintf( f, "  imagenes:\n    tablero:\n      height: %d\n      width: %d\n", w, h  );
            fprintf( f, "      imagen: !!binary |\n" );
            char* data, *cursor, *final;
            size_t  total = base64_encode_alloc( (char*) png, (size_t)size, &data );
            if( total == 0 || data == NULL ){
                LOGPRINT( 1, "Error al intentar decodificar png %d", 0 );
                fclose( f );
                render_500( conn, ri, "Error al intentar decodificar png" );
                return ;
            }
            final = data + total;
            cursor = data;
            while( cursor < final ){
                fprintf( f, "        %.50s\n", cursor );
                cursor += 50;
            }
            free( data );
            qgames_free_png( png );
            i = 1;
            fprintf( f, "    piezas:\n" );
            while( tpieza = qg_tipojuego_info_tpieza( tj, i ) ){
                int c = 1;
                while( color = qg_tipojuego_info_color( tj, c ) ){
                    if( size = qg_tipojuego_get_tpieza_png( tj, color, tpieza, 0, &png, &w, &h ) ){
                        fprintf( f, "    - tipo_pieza: %s\n", tpieza );
                        fprintf( f, "      color: %s\n", color );
                        fprintf( f, "      width: %d\n", w );
                        fprintf( f, "      height: %d\n", h );
                        fprintf( f, "      imagen: !!binary |\n" );
                        size_t  total = base64_encode_alloc( (char*) png, (size_t)size, &data );
                        if( total == 0 || data == NULL ){
                            LOGPRINT( 1, "Error al intentar decodificar png %d", 0 );
                            render_500( conn, ri, "Error al intentar decodificar png" );
                            return ;
                        }
                        final = data + total;
                        cursor = data;
                        while( cursor < final ){
                            fprintf( f, "        %.50s\n", cursor );
                            cursor += 50;
                        }
                        free( data );
                        qgames_free_png( png );
                    }
                    if( size = qg_tipojuego_get_tpieza_png( tj, color, tpieza, GETPNG_PIEZA_CAPTURADA, &png, &w, &h ) ){
                        fprintf( f, "    - tipo_pieza: %s\n", tpieza );
                        fprintf( f, "      capturada: true\n" );
                        fprintf( f, "      color: %s\n", color );
                        fprintf( f, "      width: %d\n", w );
                        fprintf( f, "      height: %d\n", h );
                        fprintf( f, "      imagen: !!binary |\n" );
                        size_t  total = base64_encode_alloc( (char*) png, (size_t)size, &data );
                        if( total == 0 || data == NULL ){
                            LOGPRINT( 1, "Error al intentar decodificar png %d", 0 );
                            render_500( conn, ri, "Error al intentar decodificar png" );
                            return ;
                        }
                        final = data + total;
                        cursor = data;
                        while( cursor < final ){
                            fprintf( f, "        %.50s\n", cursor );
                            cursor += 50;
                        }
                        free( data );
                        qgames_free_png( png );
                    }
                    c ++;
                }
                i ++;
            }
        } else {
            LOGPRINT( 5, "Error al intentar obtener png %s", gt->nombre );
        }
    }
    game_type_end( &cursor );
    render_200f( conn, ri, f );
    close( f );
}


/*
 * Dado un juego existente en la base, devuelve el PNG asociado
 * */
static void  game_controller_imagen( struct mg_connection* conn, const struct mg_request_info* ri, Session* s, char* id ){
    
    Game*  g = game_load( id );
    if( !g ){
        render_404( conn, ri );
        return;
    }
    if( !game_check_user( g, session_user( s ) ) ){
        render_500( conn, ri, "Usuario no autorizado" );
        game_free( g );
        return;
    }
    char* v;
    int  flags = GETPNG_HIGHLIGHT_RED;
    int  move_number = LAST_MOVE;
    void* png;

    if( v = mg_get_var( conn, "n" )){
        move_number = atol( v );
        free( v );
    }
  
    if( v = mg_get_var( conn, "r" )){
        flags |= GETPNG_ROTADO;
        free( v );
    }

    int size = qg_partida_get_png( game_partida( g ), flags, move_number, &png );
    if( !size ){
        render_500( conn, ri, "No es posible dibujar la partida" );
        return;
    }

    
		mg_printf(conn,
		    "HTTP/1.1 200 OK\r\n"
		    "Content-Type: image/png\r\n"
		    "Content-Length: %d\r\n"
		    "Connection: close\r\n"
		    "\r\n", size );

    mg_write( conn, png, size );
    qgames_free_png( png );
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
        case  ACTION_HISTORIAL:
            game_controller_historial( conn, ri, s, parm );
            break;
        case  ACTION_POSIBLES:
            game_controller_posibles( conn, ri, s, parm );
            break;
        case  ACTION_MUEVE:
            game_controller_mueve( conn, ri, s, parm );
            break;
        case  ACTION_REGISTRA:
            game_controller_registra( conn, ri, s, parm );
            break;
        case  ACTION_REGISTRACIONES:
            game_controller_registraciones( conn, ri, s);
            break;
        case  ACTION_DESREGISTRA:
            game_controller_desregistra( conn, ri, s, parm );
            break;
        case  ACTION_PARTIDA:
            game_controller_partida( conn, ri, s, parm );
            break;
        case  ACTION_TIPOJUEGOS:
            game_controller_tjuegos( conn, ri, s, parm[0] ? parm : NULL );
            break;
        case  ACTION_IMAGEN:
            game_controller_imagen( conn, ri, s, parm );
            break;
        default:
            render_404( conn, ri );
    }

}

/* vi: set cin sw=4: */ 
