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

#include  "users.h"
#include  "games.h"
#include  "dbmanager.h"

#include  "log.h"

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
GameType*  bin_to_game_type( void* data, int size ){
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
        LOGPRINT( 5, "Error salvando tipo de juego %d (%s)", gt->id, db_getlasterror() );
        return 0;
    }
    gt->rec_flags &= ~RECFLAG_NEW;

    return 1;

}
