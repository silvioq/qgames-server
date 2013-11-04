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
#include  <stdlib.h>
#include  "mongoose.h"
#include  "users.h"
#include  "games.h"
#include  "log.h"
#include  "session.h"
#include  "base64.h"
#include  "cJSON.h"
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

static  cJSON* game_to_cJSON( Game* g, Partida* p ){
    char* res;
    int  movidas;
    cJSON* game = cJSON_CreateObject();
    cJSON_AddStringToObject( game, "game_id", g->id );
    cJSON_AddStringToObject( game, "tipo_juego", game_game_type( g )->nombre );
    if( p ){
        cJSON_AddStringToObject( game, "color", qg_partida_color( p ) );
        movidas = qg_partida_movhist_count( p );
        cJSON_AddNumberToObject( game, "cantidad_movidas", movidas );
        qg_partida_final( p, &res );
        cJSON_AddStringToObject( game, "descripcion_estado", res ? res : "Jugando" );
        cJSON_AddBoolToObject( game, "es_continuacion", qg_partida_es_continuacion( p ) );

        if( movidas > 0 ){
            int i = 0, len = 0;
            char destino[180] = "";
            while( res = (char*) qg_partida_movhist_destino( p, movidas - 1, i ) ){
                if( strlen( res ) + len + 1 > 180 ) break;
                if( i )
                    sprintf( destino, "%s,%s", destino, res );
                else
                    strcpy( destino, res );
                i ++;
            }
            cJSON_AddStringToObject( game, "ultimos_destino", destino );
        }
    } else {
        cJSON_AddStringToObject( game, "color", g->color ? g->color : "-" );
        cJSON_AddNumberToObject( game, "cantidad_movidas", g->cantidad_movidas );
        cJSON_AddNumberToObject( game, "final_estado", g->final );
        cJSON_AddStringToObject( game, "descripcion_estado", g->estado ? g->estado : "-" );
        cJSON_AddBoolToObject( game, "es_continuacion", g->es_continuacion );
        if( g->notacion )
            cJSON_AddStringToObject( game, "ultima_jugada", g->notacion );
        if( g->destino )
            cJSON_AddStringToObject( game, "ultimos_destino", g->destino );
    }
    return game;
}



static void  game_controller_crea( struct mg_connection* conn, const struct mg_request_info* ri, Session* s, char* game_type, int format ){

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

    Game* g2 = game_load( g->id );   // FIXME: Es necesario volver a leer. Corregir la funcion game_save para
                                     //        que quede como corresponde
    game_free( g );
    g = g2;    
    cJSON* gson = game_to_cJSON( g, NULL );
    game_free( g );
    render_200j( conn, ri, gson, format );
    cJSON_Delete( gson );
    
}

static void  game_controller_tablero( struct mg_connection* conn, const struct mg_request_info* ri, Session* s, char* id, int format ){
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
    cJSON* gson = game_to_cJSON( g, p );
    
    int pie = qg_partida_tablero_count( p, LAST_MOVE );
    int cap = qg_partida_tablero_countcap( p, LAST_MOVE );
    cJSON_AddNumberToObject( gson, "total", pie );
    cJSON* piezas = cJSON_CreateArray( );
    for( i = 0; i < pie; i ++ ){
        char* casillero; char* tipo; char* color;
        qg_partida_tablero_data( p, LAST_MOVE, i, &casillero, &tipo, &color );

        cJSON* itemp = cJSON_CreateObject( );
        cJSON_AddStringToObject( itemp, "casillero", casillero );
        cJSON_AddStringToObject( itemp, "tipo", tipo );
        cJSON_AddStringToObject( itemp, "color", color );
        cJSON_AddItemToArray( piezas, itemp );
    }
    for( i = 0; i < cap; i ++ ){
        char* tipo; char* color;
        qg_partida_tablero_datacap( p, LAST_MOVE, i, &tipo, &color );
        cJSON* itemp = cJSON_CreateObject( );
        cJSON_AddStringToObject( itemp, "casillero", ":captured" );
        cJSON_AddStringToObject( itemp, "tipo", tipo );
        cJSON_AddStringToObject( itemp, "color", color );
        cJSON_AddItemToArray( piezas, itemp );
    }
    cJSON_AddItemToObject( gson, "piezas", piezas );

    game_free( g );
    render_200j( conn, ri, gson, format );
    cJSON_Delete( gson );
}


static cJSON*  movida_to_cJSON( Partida* p, Movdata movd ){
    cJSON* mov = cJSON_CreateObject( );
    cJSON_AddNumberToObject( mov, "numero", movd.numero );
    cJSON_AddStringToObject( mov, "descripcion", movd.descripcion );
    cJSON_AddStringToObject( mov, "pieza", movd.pieza );
    cJSON_AddStringToObject( mov, "color", movd.color );
    if( movd.captura ){
        cJSON_AddBoolToObject( mov, "es_captura", 1 );
        cJSON_AddStringToObject( mov, "captura", movd.captura_pieza );
        cJSON_AddStringToObject( mov, "captura_cas", movd.captura_casillero );
    }
    cJSON_AddStringToObject( mov, "origen", ( movd.origen == CASILLERO_POZO ? "" : movd.origen ) );
    cJSON_AddStringToObject( mov, "destino" , movd.destino );
    cJSON_AddStringToObject( mov, "notacion" , movd.notacion );
    if( movd.transforma ){
        cJSON_AddStringToObject( mov, "transforma_tipo", movd.transforma_pieza );
        cJSON_AddStringToObject( mov, "transforma_color", movd.transforma_color );
    }
    if( movd.movida ){
        cJSON* detalles = cJSON_CreateArray( );
        do{
            cJSON* det = cJSON_CreateObject( );
            cJSON_AddStringToObject( det, "origen", ( movd.movida_origen == CASILLERO_POZO ? "" : movd.movida_origen ) );
            cJSON_AddStringToObject( det, "destino", ( movd.movida_destino == CASILLERO_POZO ? "" : movd.movida_destino ) );
            cJSON_AddItemToArray( detalles, det );
        } while( qg_partida_movdata_nextmov( p, &movd ) );
        cJSON_AddItemToObject( mov, "detalle", detalles );
    }

    if( movd.crea ){
        cJSON* crea = cJSON_CreateArray( );
        do {
            cJSON* c = cJSON_CreateObject( );
            cJSON_AddStringToObject( c, "pieza", movd.crea_pieza );
            cJSON_AddStringToObject( c, "casillero", movd.crea_casillero );
            cJSON_AddStringToObject( c, "color", movd.crea_color );
            cJSON_AddItemToArray( crea, c );
        } while( qg_partida_movdata_nextcrea( p, &movd ) );
        cJSON_AddItemToObject( mov, "crea", crea );
    }
    return mov;
}


static void  game_controller_historial( struct mg_connection* conn, const struct mg_request_info* ri, Session* s, char* id, int format ){
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
    cJSON* gson = game_to_cJSON( g, p );
    cJSON* tabinicial = cJSON_CreateArray();
    cJSON* movidas    = cJSON_CreateArray();
   
    i = 0;
    while( true ){
        char* casillero; char* tipo; char* color;
        if( !qg_partida_tablero_data( p, 0, i, &casillero, &tipo, &color ) ) break;
        cJSON* itemp = cJSON_CreateObject( );
        cJSON_AddStringToObject( itemp, "casillero", casillero );
        cJSON_AddStringToObject( itemp, "tipo", tipo );
        cJSON_AddStringToObject( itemp, "color", color );
        cJSON_AddItemToArray( tabinicial, itemp );
        i ++;
    }

    i = 0;
    while( qg_partida_movhist_data( p, i, &movd ) ){
        cJSON_AddItemToArray( movidas,
               movida_to_cJSON( p, movd ) );
        i ++;
    }

    cJSON_AddItemToObject( gson, "tablero_inicial", tabinicial );
    cJSON_AddItemToObject( gson, "movidas", movidas );

    game_free( g );
    render_200j( conn, ri, gson, format );
    cJSON_Delete( gson );

}

/*
 * Realiza la movida pasada como parametro
 * */
static void  game_controller_mueve( struct mg_connection* conn, const struct mg_request_info* ri, Session* s, char* id, int format ){
    if( strcmp( ri->request_method, "POST" ) != 0 ){
        render_400( conn, ri, "Debe enviar movida (m=xxxx) por POST" );
        return;
    }
    char* move = get_var( conn, ri, "m" );
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
    if( !p ){
        LOGPRINT( 2, "Error leyendo partida. Game: %s", id );
        render_500( conn, ri, "Error leyendo partida" );
        game_free( g );
        free( move );
        return;
    }
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

    game_set_partida( g, p );
    if(!game_save( g ) ){
        LOGPRINT( 5, "Error de partida!", 0 );
        render_400( conn, ri, "Error al guardar partida" );
        return;
    } else {
        dbact_sync();
    }

    cJSON* gson = game_to_cJSON( g, p );
    render_200j( conn, ri, gson, format );
    cJSON_Delete( gson );
    free( move );
    game_free( g );
}


/*
 * esta es la accion que devuelve aquellos movimientos posibles
 * a realizar
 * */
static void  game_controller_posibles( struct mg_connection* conn, const struct mg_request_info* ri, Session* s, char* id, int format ){
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
    if( !p ){
        LOGPRINT( 2, "Error intentando leer partida %s", id );
        render_500( conn, ri, "Error al intentar leer partida" );
        game_free( g );
        return;
    }
    if( !save_game_if_not_calculed( g, p ) ){
        render_500( conn, ri, "Error al intentar recalcular partida" );
        game_free( g );
        return;
    }

    int i;
    cJSON* gson = game_to_cJSON( g, p );
    cJSON* movidas    = cJSON_CreateArray();
    
    int pie = qg_partida_movidas_count( p );
    cJSON_AddNumberToObject( gson, "total", pie );
    for( i = 0; i < pie; i ++ ){
        Movdata movd;
        if( qg_partida_movidas_data( p, i, &movd ) ){
            cJSON_AddItemToArray( movidas, movida_to_cJSON( p, movd ) );
        }
    }
    cJSON_AddItemToObject( gson, "movidas", movidas );

    render_200j( conn, ri, gson, format );
    cJSON_Delete( gson );
    game_free( g );
}

/*
 * Esta accion permite registrar un juego a partir de su binario
 * */
static void  game_controller_registra( struct mg_connection* conn, const struct mg_request_info* ri, Session* s, char* id, int format ){
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
    void*  post_data; int post_len;
    if( !get_post_data( conn, ri, &post_data, &post_len ) ){
        render_500( conn, ri, "Peticion incorrecta, no puede tomarse datos de metodo POST" );
        game_free( g );
        return ;
    }
    char*  game_type = strdup( qg_partida_load_tj( post_data, post_len ) );
    LOGPRINT( 5, "recibo por post %d => %s", post_len, game_type );
    GameType* gt = game_type_share_by_name( game_type );
    if( !gt ){
        LOGPRINT( 2, "Error al intentar encontrar el tipo de juego %s", game_type );
        free( game_type );
        render_500( conn, ri, "No se puede encontrar el tipo de juego" );
        return;
    }
    free( game_type );

    Partida* p = qg_partida_load( game_type_tipojuego( gt ), post_data, post_len );
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


    cJSON* gson = game_to_cJSON( g, p );
    render_200j( conn, ri, gson, format );
    cJSON_Delete( gson );
    game_free( g );

}

/*
 * Dado un juego existente en la base, se desregistra o elimina
 * */
static void  game_controller_desregistra( struct mg_connection* conn, const struct mg_request_info* ri, Session* s, char* id ){
    
    if( strcmp( ri->request_method, "POST" ) != 0 ){
        render_400( conn, ri, "Debe enviar deregistracion por POST" );
        return;
    }
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
    render_200( conn, ri, NULL, "Partida desregistrada" );
}

/*
 * Dado un juego existente en la base, devuelve el binario asociado
 * */
static void  game_controller_partida( struct mg_connection* conn, const struct mg_request_info* ri, Session* s, char* id, int format ){
    
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

    cJSON* gson;
    if( format == FORMAT_QGAME ){
        gson = cJSON_CreateObject();
        cJSON_AddStringToObject( gson, "game_id", id );
        cJSON_AddBinaryToObject( gson, "data", g->data, g->data_size );
    } else {
        gson = game_to_cJSON( g, NULL );
    }
   
    render_200j( conn, ri, gson, format );

    cJSON_Delete( gson ); 
    game_free( g );

}

/*
 * En esta accion se devuelven los juegos activos del usuario
 * que esta en la sesion
 * */
static void  game_controller_registraciones( struct mg_connection* conn, const struct mg_request_info* ri, Session* s, int format ){
    void* games = NULL;
    int  cantidad = 0;
    Game* g ;
    User* u = session_user( s );
    cJSON* data = cJSON_CreateObject();
    cJSON* lista = cJSON_CreateArray();
    while( game_next( &games, &g ) ){
        if( game_check_user( g, u ) ){
            cJSON_AddItemToArray( lista, game_to_cJSON( g, NULL ) );
            cantidad ++;
        }
        game_free( g );
    }
    game_type_end( &games );

    cJSON_AddNumberToObject( data, "cantidad", cantidad );
    cJSON_AddStringToObject( data, "respuesta", "OK" );
    char total[50];
    sprintf( total, "Cantidad de juegos: %d", cantidad );
    cJSON_AddStringToObject( data, "texto", total );
    cJSON_AddItemToObject( data, "games", lista );
    
    render_200j( conn, ri, data, format );
    cJSON_Delete( data );
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

    if( v = get_var( conn, ri, "n" )){
        move_number = atol( v );
        free( v );
    }
  
    if( v = get_var( conn, ri, "r" )){
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
void game_controller( struct mg_connection* conn, const struct mg_request_info* ri, Session* s, int action, int format, char* parm ){
    session_save( s ); // toco la sesion
    switch(action){
        case  ACTION_CREA:
            game_controller_crea( conn, ri, s, parm, format );
            break;
        case  ACTION_TABLERO:
            game_controller_tablero( conn, ri, s, parm, format );
            break;
        case  ACTION_HISTORIAL:
            game_controller_historial( conn, ri, s, parm, format );
            break;
        case  ACTION_POSIBLES:
            game_controller_posibles( conn, ri, s, parm, format );
            break;
        case  ACTION_MUEVE:
            game_controller_mueve( conn, ri, s, parm, format );
            break;
        case  ACTION_REGISTRA:
            game_controller_registra( conn, ri, s, parm, format );
            break;
        case  ACTION_REGISTRACIONES:
            game_controller_registraciones( conn, ri, s, format);
            break;
        case  ACTION_DESREGISTRA:
            game_controller_desregistra( conn, ri, s, parm );
            break;
        case  ACTION_PARTIDA:
            game_controller_partida( conn, ri, s, parm, format );
            break;
        case  ACTION_IMAGEN:
            game_controller_imagen( conn, ri, s, parm );
            break;
        default:
            render_404( conn, ri );
    }

}

/* vi: set cin sw=4: */ 
