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
#include  <malloc.h>
#include  <string.h>
#include  <config.h>
#include  <stdint.h>
#include  <sys/time.h>
#include  <time.h>
#include  <qgames.h>

#include  <sys/types.h>
#include  <dirent.h>
#include  <errno.h>
#include  <pthread.h>


#include  "users.h"
#include  "game_types.h"
#include  "games.h"
#include  "dbmanager.h"

#include  "log.h"


static  GameType** game_types_lista = NULL;
static  int        game_types_lista_count = 0;
static  int        game_types_lista_alloc = 0;
extern  pthread_mutex_t lector_mutex;

#define   RECFLAG_NEW   0x01


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
 * Accedo al tipo de juego por su identificador
 * */
GameType* game_type_load( unsigned int id ){
    void* data;
    int  size, ret;
    ret = dbget_data( DBGAMETYPE, &id, sizeof( id ), &data, &size ) ;
    if( ret == 0 ){
        LOGPRINT( 5, "Tipojuego %d no encontrado", id );
        return NULL;
    } else if( ret == -1 ){
        LOGPRINT( 1, "Error de base de datos %s", dbget_lasterror() );
        return NULL;
    } else {
        return  bin_to_game_type( data, size );
    }
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
    free( data );
    if( ret == 0 ){
        LOGPRINT( 5, "Error salvando tipo de juego %d (%s)", gt->id, dbget_lasterror() );
        return 0;
    }
    gt->rec_flags &= ~RECFLAG_NEW;

    return 1;

}


/*
 * Uso del cursor:
 *
 *    void * cursor = NULL; // inicializar en nulo
 *    GameType* gt; 
 *    while( game_type_next( &cursor, &gt ) ){
 *      // do something ...
 *    }
 *    game_type_end( &cursor );
 * */
int        game_type_next( void** cursor, GameType** gt ){

    void* dbc = (*cursor);
    if( !dbc ){
        dbc = dbcur_new( DBGAMETYPE );
        *cursor = dbc;
    }

    void* data;
    int   size;

    int ret = dbcur_get( dbc, DBNEXT, &data, &size );
    if( ret == 0 ){
        if( gt ) *gt = NULL;
        return 0;
    } else if( ret == -1){
        LOGPRINT( 1, "Error al intentar leer data %s", dbget_lasterror() );
        if( gt ) *gt = NULL;
        return 0;
    }
    GameType* game_temp = bin_to_game_type( data, size );
    if( game_temp ){
        if( gt ){ 
            *gt = game_type_share_by_id( game_temp->id, game_temp );
            if( *gt != game_temp ) game_type_free( game_temp );
        } else {
            game_type_free( game_temp );
        }
        return 1;
    }

    if( gt ) *gt = NULL;
    return 0;
}

/*
 * Finaliza el cursor
 * */
int        game_type_end( void** cursor ){
    dbcur_end( *cursor );
}

/*
 * Esta funcion recorre el directorio de los tipos de juego e intenta crear todos 
 * los que encuentre alli
 * */
int        game_type_discover( ){
    const char* dirname = qg_path_games( );
    DIR*  d = opendir( dirname );
    struct  dirent*  ent;
    if( !d ){
        LOGPRINT( 2, "Error al abrir directorio %s => %d %s", dirname, errno, strerror( errno ) );
        return 0;
    }

    while( ent = readdir( d ) ){
        char nombre[256];
        strcpy( nombre, ent->d_name );
        char* aux = strstr( nombre, ".qgame" );
        if( !aux ) { continue; }
        aux[0] = 0;
        LOGPRINT( 5, "Definicion encontrada => %s", nombre );
        if( !game_type_share_by_name( nombre ) ){
            LOGPRINT( 2, "Error al querer compartir %s", nombre );
            return 0;
        }
    }
    closedir( d );
    return 1;

    
}



/*
 * Dado un tipo de juego, verifica si se encuentra en memoria.
 * En el caso positivo, lo devuelve. Si no esta en memoria, 
 * intenta leerlo de la base. Si aun no lo encuentra, entonces
 * verifica su existencia y lo graba.
 * */
GameType*  game_type_share_by_name( char* name ){
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
    ret = game_type_new( name, 0 );
    Tipojuego* tj = game_type_tipojuego( ret );
    if( !tj ) return NULL;

    game_type_save( ret );
    game_types_lista[game_types_lista_count++] = ret;
    return ret;

}


/*
 * Dado un id de tipo de juego, intenta verificar si esta en lo compartido
 * En el caso que no este, verifica en la base de datos. 
 * Si el parametro game_type_loaded no es nulo, entonces se usa ese objeto
 * Si no esta, devuelve nulo
 * */
GameType*  game_type_share_by_id( unsigned int id, GameType* game_type_loaded ){
    int i;
    // Verifico si esta en memoria
    for( i = 0; i < game_types_lista_count; i ++ ){
        if( id == game_types_lista[i]->id ) return game_types_lista[i];
    }

    // Voy a necesitar espacio en memoria, lo creo
    if( game_types_lista_count >= game_types_lista_alloc ){
        game_types_lista_alloc += 10;
        game_types_lista = realloc( game_types_lista, game_types_lista_alloc * sizeof( GameType* ) );
    }

    // Verifico si esta en la base
    GameType* ret = game_type_loaded ? game_type_loaded : game_type_load( id );
    if( ret ){
        game_types_lista[game_types_lista_count++] = ret;
        return ret;
    }

    // Levanto el tipo de juego
    return NULL;
}

/*
 * Limpieza de cache
 *
 */
void   game_type_share_clean(){
    int i;
    for( i = 0; i < game_types_lista_count; i ++ ){
        game_type_free( game_types_lista[i] );
    }
    if( game_types_lista )
        free( game_types_lista );
    game_types_lista_alloc = 0;
    game_types_lista_count = 0;
}


/*
   Devuelve el tipo de juego Tipojuego* a partir del
   objeto GameType
*/
Tipojuego*  game_type_tipojuego( GameType* gt ){
    pthread_mutex_lock( &lector_mutex );
    if( !gt->tipojuego ){
        LOGPRINT( 5, "Intentando leer definiciones de juego %s", gt->nombre );
        gt->tipojuego = qg_tipojuego_open( gt->nombre );
        LOGPRINT( 5, "Juego %s %s", gt->nombre, ( gt->tipojuego ? "OK" : "ERROR" )  );
        if( !gt->tipojuego ){
            LOGPRINT( 1, "Error al intentar leer las reglas de %s", gt->nombre );
            pthread_mutex_unlock( &lector_mutex );
            return 0;
        }
    }
    pthread_mutex_unlock( &lector_mutex );
    return gt->tipojuego;
    
}


// vim: set cin sw=4:
