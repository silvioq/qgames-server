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

#include  "users.h"
#include  "games.h"
#include  "dbmanager.h"

#include  "log.h"

#define   RECFLAG_NEW   0x01

static  GameType** game_types_lista = NULL;
static  int       game_types_lista_count = 0;
static  int       game_types_lista_alloc = 0;



/*
 * Crea un nuevo tipo de juego.
 * */
GameType*  game_type_new( char* name, time_t created_at ){
    GameType* gt = malloc( sizeof( GameType ) );
    memset( gt, 0, sizeof( GameType ) );
    gt->nombre = strdup( name );
    if( created_at )
        gt->created_at = created_at;
    else {
        struct timeval tv;
        gettimeofday( &tv, NULL );
        gt->created_at = tv.tv_sec ;
    }
        
    gt->rec_flags |= RECFLAG_NEW;
    return gt;
}

/*
 * Limpiamos la memoria asignada
 * */
void       game_type_free( GameType* gt ){
    if( gt->nombre ) free( gt->nombre );
    free( gt );
}

/*
 * Simple serializador
 * */
static int game_type_to_bin( GameType* gt, void** data ){
    int size;
    if( binary_pack( "isl", data, &size, gt->id, gt->nombre, (long)gt->created_at ) ){
        return size;
    } else return 0;
}

/*
 * Deserialización del tipo de juego
 * */
static  GameType*  bin_to_game_type( void* data, int size ){
    int id;
    char* nombre;
    time_t  created_at;
    if( binary_unpack( "isl", data, size, &id, &nombre, &created_at ) ){
        GameType* gt = game_type_new( nombre, created_at );
        gt->id = id;
        gt->rec_flags &= ~RECFLAG_NEW;
        return gt;
    } else {
        return NULL;
    }
}


/*
 * Accedo al tipo de juego por su nombre
 * */
GameType*  game_type_by_name( char* nombre ){
    void* data;
    int  size;
    if( dbget_data( IDXGAMETYPENAME, nombre, strlen( nombre ), &data, &size ) ){
        return bin_to_game_type( data, size );
    }
    return NULL;
}

/*
 * Con esta funcion salvo el tipo de juego
 * */
int        game_type_save( GameType* gt ){
    if( gt->rec_flags | RECFLAG_NEW ){
        gt->id = dbget_game_typenextid();
    }
    void* data;
    int  size;

    size = game_type_to_bin( gt, &data );
    if( size == 0 ){
        LOGPRINT( 1, "Error en dump de tuoi de juego %d", gt->id );
        return 0;
    }

    int ret;
    ret = dbput_data( DBGAMETYPE, &gt->id, sizeof( gt->id ), data, size );
    if( ret == 0 ){
        LOGPRINT( 5, "Error salvando tipo de juego %d (%s)", gt->id, dbget_lasterror() );
        return 0;
    }
    gt->rec_flags &= ~RECFLAG_NEW;

    return 1;

}

/*
 * Dado un tipo de juego, verifica si se encuentra en memoria.
 * En el caso positivo, lo devuelve. Si no esta en memoria, 
 * intenta leerlo de la base. Si aun no lo encuentra, entonces
 * verifica su existencia y lo graba.
 * */
GameType*  game_type_load( char* name ){
    int i;

    // Verifico si esta en memoria
    for( i = 0; i < game_types_lista_count; i ++ ){
        if( strcmp( game_types_lista[i]->nombre, name ) == 0 ) return game_types_lista[i];
    }

    // Voy a necesitar espacio en memoria, lo creo
    if( game_types_lista_count >= game_types_lista_alloc ){
        game_types_lista_alloc += 10;
        game_types_lista = realloc( game_types_lista, game_types_lista_alloc * sizeof( GameType* ) );
    }

    // Verifico si esta en la base
    GameType* ret = game_type_by_name( name );
    if( ret ){
        game_types_lista[game_types_lista_count++] = ret;
        return ret;
    }

    // Voy a ver si hay algo!
    Tipojuego* tj = qg_tipojuego_open( name );
    if( !tj ) return NULL;

    ret = game_type_new( name, 0 );
    ret->tipojuego = tj;
    game_type_save( ret );
    game_types_lista[game_types_lista_count++] = ret;
    return ret;

}


/*
 *
 *
 * A partir de aqui vienen las funciones de Game.
 * Lo primero es la serializacion, para lo cual usamos las 
 * maravillosas funciones de empaquetado
 *
 *
 * */
static void  game_set_partida( Game* g, Partida* p ){
    void* data;
    int   size;
    qg_partida_dump( p, &data, &size );
    game_set_data( g, data, size );
}


static int game_to_bin( Game* g, void** data ){
    int size;
    if( binary_pack( "siibl", data, &size, g->id, g->user_id, g->game_type_id, 
                            g->data, g->data_size, 
                            (long)g->created_at ) ){
        return size;
    } else return 0;
}

/*
 * Dado el binario empaquetado, obtenermos el juego
 * */

static Game* bin_to_game( void* data, int size ){
    char* id;
    unsigned int user_id;
    unsigned int game_type_id;
    void* gdata;
    int   data_size;
    time_t  created_at;
    if( binary_unpack( "siibl", data, size, &id, &user_id, &game_type_id,
                          &gdata, &data_size, &created_at ) ){
        Game* g = game_new( id, NULL, NULL, 0 );
        g->user_id = user_id;
        g->game_type_id = game_type_id;
        if( gdata ) game_set_data( g, gdata, data_size );
        g->created_at = created_at;
        g->rec_flags &= ~RECFLAG_NEW;
        return g;
      
    } else return NULL;
}

/*
 * Esta es la creacion del juego
 * */
Game*   game_new( char* id, User* user, GameType* type, time_t created_at ){
    Game* g = malloc( sizeof( Game ) );
    memset( g, 0, sizeof( Game ) );
    g->id = strdup( id );
    g->user = user;
    if( user ) g->user_id = user->id;
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
    if( game->data ) free( game->data );
    if( game->partida ) qg_partida_free( game->partida );
    free( game );
}


/*
 *
 * Esta funcion crea una nueva partida y setea, como corresponde
 * el nuevo objeto game
 *
 * */

Game*     game_type_create( GameType* gt, User* u ){
    if( !gt->tipojuego ){
        gt->tipojuego = qg_tipojuego_open( gt->nombre );
        if( !gt->tipojuego ){
            LOGPRINT( 1, "Error al intentar leer las reglas de %s", gt->nombre );
            return 0;
        }
    }
    Partida* p = qg_tipojuego_create_partida( gt->tipojuego, NULL );
    Game* ret = game_new( qg_partida_id( p ), u, gt, 0 );
    game_set_partida( ret, p );
    qg_partida_free( p );
    return ret;
    

}
