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

#include  <malloc.h>
#include  <string.h>
#include  <config.h>
#include  <stdint.h>
#include  <sys/time.h>
#include  <time.h>
#include  <qgames.h>

#include  <sys/types.h>
#include  <errno.h>
#include  <pthread.h>


#include  "users.h"
#include  "game_types.h"
#include  "games.h"
#include  "dbmanager.h"

#include  "log.h"


pthread_mutex_t lector_mutex = PTHREAD_MUTEX_INITIALIZER;

#define   RECFLAG_NEW   0x01


/*
 * A partir del dato Partida* (qgames) se setea toda la informacion
 * del juego
 * */
void  game_set_partida( Game* g, Partida* p ){
    if( g->data ) free( g->data );
    if( g->color ) free( g->color );
    if( g->estado ) free( g->estado );
    if( g->destino ) free( g->destino );
    if( g->notacion ) free( g->notacion );
    qg_partida_dump( p, &g->data, &g->data_size );
    g->color = strdup( qg_partida_color( p ) );
    char* res;
    g->final  = qg_partida_final( p, &res );
    g->estado = strdup( res ? res : "Jugando" );
    g->es_continuacion = qg_partida_es_continuacion( p );

    int movidas = qg_partida_movhist_count( p );
    g->cantidad_movidas = movidas;
    Movdata  movd;
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
        g->destino = strdup( destino );
        if( qg_partida_movhist_data( p, -1, &movd ) )
            g->notacion = strdup( movd.notacion );
        else {
            LOGPRINT( 1, "Error accediendo a notacion de partida %s", g->id );
            g->notacion = NULL;
        }

    } else {
        g->destino = NULL;
        g->notacion = NULL;
    }
}





/*
 * Esta es la creacion del juego
 * */
Game*   game_new( char* id, User* user, GameType* type, time_t created_at ){
    Game* g = malloc( sizeof( Game ) );
    memset( g, 0, sizeof( Game ) );
    g->id = strdup( id );
    if( user ){
        g->user_id = user->id;
        g->user = user_dup( user );
    }
    g->game_type = type;
    if( type ) g->game_type_id = type->id;

    if( created_at )
        g->created_at = created_at;
    else {
        struct timeval tv;
        gettimeofday( &tv, NULL );
        g->created_at = tv.tv_sec ;
    }
        
    g->rec_flags |= RECFLAG_NEW;
    return g;
}


/*
 * Seteo los datos del juego
 * */
void    game_set_data( Game* g, void* data, unsigned int data_size ){
    g->data = realloc( g->data, data_size );
    memcpy( g->data, data, data_size );
    g->data_size = data_size ;
}

/*
 * Libera el espacio de memoria usado
 * */
void    game_free( Game* game ){
    if( game->id )  free( game->id );
    if( game->data ) free( game->data );
    if( game->partida ) qg_partida_free( game->partida );
    if( game->color ) free( game->color );
    if( game->estado ) free( game->estado );
    if( game->destino ) free( game->destino );
    if( game->notacion ) free( game->notacion );
    if( game->user ) user_free( game->user );
    free( game );
}


/*
 * Serializacion de juego
 * */

static  int  game_to_bin( Game* g, void** data ){
    int  size;
    volatile char* null = "";
    if( binary_pack( "siibssssccill", data, &size, 
                g->id, g->user_id, g->game_type_id, g->data, g->data_size, 
                g->color ? g->color : null, g->estado ? g->estado : null, g->destino ? g->destino : null, g->notacion ? g->notacion : null, 
                g->es_continuacion, g->final, g->cantidad_movidas,
                g->created_at, g->modified_at ) ){
        return size;
    } else return 0;
}

/*
 * Deserializacion de juego
 * */
static Game* bin_to_game( void* data, int size ){
    char* id;
    unsigned long user_id, game_type_id;
    void*  game_data;
    int    game_data_size;
    char*  color, *estado, *notacion, *destino;
    char   es_cont, final;
    int    cantidad;
    time_t  created_at, modified_at;

    if( binary_unpack( "siibssssccill", data, size,
                &id, &user_id, &game_type_id,
                &game_data, &game_data_size,
                &color, &estado, &destino, &notacion,
                &es_cont, &final, &cantidad,
                &created_at, &modified_at ) ){
        if( game_data_size && !game_data ){
            LOGPRINT( 1, "Error en lectura de juego. Tamaño incompatible con datos: Size %d", game_data_size );
            return NULL;
        }
        Game* g = game_new( id, NULL, NULL, created_at );
        g->game_type_id = game_type_id;
        g->user_id      = user_id;
        g->color        = color[0] ? strdup( color ) : NULL;
        g->estado       = estado[0] ? strdup( estado ) : NULL;
        g->destino      = destino[0] ? strdup( destino ) : NULL;
        g->notacion     = notacion[0] ? strdup( notacion ) : NULL;
        g->es_continuacion = es_cont;
        g->final        = final;
        g->cantidad_movidas = cantidad;
        g->modified_at  = modified_at;
        if( game_data_size ) game_set_data( g, game_data, game_data_size );
        return g;
    } else return NULL;

}


/*
 * Lectura de un juego de la base
 * */
Game*   game_load( char* id ) {
    void * data; int size;
    int ret = dbget_data( DBGAME, id, strlen( id ), &data, &size );
    if( !ret ){
        LOGPRINT( 5, "Game no encontrado %s", id );
        return NULL;
    } else if( ret == -1 ){
        LOGPRINT( 1, "Error de base de datos %s", dbget_lasterror() );
        return NULL;
    } else {
        LOGPRINT( 6, "Game loaded %s", id );
    }
    Game* g = bin_to_game( data, size );
    if( !g ){
        LOGPRINT( 2, "Error decodificando juego %s", id );
        return NULL;
    }
    g->rec_flags &= ~RECFLAG_NEW;
    return g;
}


/*
 * Grabando un nuevo juego!
 * */
int  game_save( Game* g ){
    struct timeval tv;
    gettimeofday( &tv, NULL );
    if( !g->created_at ) g->created_at = tv.tv_sec;
    g->modified_at = tv.tv_sec;
    
    void* data;
    int size = game_to_bin( g, &data );
    if( size ){
        if( !dbput_data( DBGAME, g->id, strlen( g->id ), data, size ) ){
            LOGPRINT( 1, "Error salvando juego %s (%s)", g->id, dbget_lasterror() );
            return 0 ;
        } else {
            LOGPRINT( 5, "Juego %s salvado!", g->id );
        }
    } else {
        LOGPRINT( 1, "Error serializando juego %s", g->id );
        return 0;
    }
    free( data );
    return 1;

}

/*
 * Elimino un juego existente
 * */
int  game_del( Game* g ){
    if( !dbdel_data( DBGAME, g->id, strlen( g->id ) ) ){
        LOGPRINT( 1, "Error eliminando juego %s (%s)", g->id, dbget_lasterror() ) ;
        return 0;
    } else {
        LOGPRINT( 5, "Juego %s eliminado", g->id );
    }
    return 1;
}

/*
 * Esta funcion devuelve uno o cero dependiendo si el usuario
 * tiene permisos o no sobre el juego.
 * En el caso que el usuario sea administrador, tiene permisos.
 * Si es un usuario comun y corriente, deberia ser el creador
 * */
int     game_check_user( Game* g, User* u ){
    if( u->tipo == USERTYPE_ADMIN ) return 1;
    return u->id == g->user_id;
}

/*
 * Devuelve la estructura de usuario asociada al juego
 * */
User*  game_user( Game* g ){
    if( g->user ) return g->user;
    if( !g->user_id ) return NULL;
    g->user = user_load( g->user_id );
    return g->user;
}

/*
 * Devuelve la estructura de tipo de juego asociada
 * */
GameType*  game_game_type( Game* g ){
    if( g->game_type ) return g->game_type;
    if( !g->game_type_id ) return NULL;
    g->game_type = game_type_share_by_id( g->game_type_id, NULL );
    return g->game_type;
}





/*
 *
 * Esta funcion crea una nueva partida y setea, como corresponde
 * el nuevo objeto game
 *
 * */

Game*     game_type_create( GameType* gt, User* u ){
    Tipojuego* tj = game_type_tipojuego( gt );
    pthread_mutex_lock( &lector_mutex );
    Partida* p = qg_tipojuego_create_partida( tj, NULL );
    Game* ret = game_new( qg_partida_id( p ), u, gt, 0 );
    game_set_partida( ret, p );
    qg_partida_free( p );
    pthread_mutex_unlock( &lector_mutex );
    return ret;
}



Partida*  game_partida( Game* g ){
    if( g->partida ) return g->partida;
    GameType* gt = game_game_type( g );
    if( !g->data ) return NULL;
    Tipojuego* tj = game_type_tipojuego( gt );
    if( !tj ){
        LOGPRINT( 2, "Error, este juego no puede resolver su tipo => %s", g->id );
        return NULL;
    }
    g->partida = qg_partida_load( tj, g->data, g->data_size );
    return g->partida;
}


/*
 * Uso del cursor:
 *
 *    void * cursor = NULL; // inicializar en nulo
 *    Game* g; 
 *    while( game_next( &cursor, &g ) ){
 *      // do something ...
 *    }
 *    game_end( &cursor );
 * */
int        game_next( void** cursor, Game** g ){

    void* dbc = (*cursor);
    if( !dbc ){
        dbc = dbcur_new( DBGAME ); // OPTIMIZE: Crear un indice por estado del partido
        *cursor = dbc;
    }

    void* data;
    int   size;

    int ret = dbcur_get( dbc, DBNEXT, &data, &size );
    if( ret == 0 ){
        if( g ) *g = NULL;
        return 0;
    } else if( ret == -1){
        LOGPRINT( 1, "Error al intentar leer data %s", dbget_lasterror() );
        if( g ) *g = NULL;
        return 0;
    }
    Game* game_temp = bin_to_game( data, size );
    if( game_temp ){
        if( g ) *g = game_temp;
        return 1;
    }

    if( g ) *g = NULL;
    return 0;
}

/*
 * Finaliza el cursor
 * */
int        game_end( void** cursor ){
    dbcur_end( *cursor );
}
/* vi: set cin sw=4: */ 
