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
#include  <sys/time.h>
#include  <time.h>
#include  <stdlib.h>

#include  "users.h"
#include  "dbmanager.h"
#include  "md5.h"
#include  "packer.h"
#include  "session.h"
#include  "log.h"

static  void session_generar_id( char id[32] ){
    
    md5_state_t  md5;
    md5_init( &md5 );
    unsigned char hash[16];

    long int r = random( );
    struct timeval tv;
    gettimeofday( &tv, NULL );
    clock_t  c = clock( );
    int i;
   
    if( tv.tv_sec & 1 ){ 
        md5_append( &md5, (char*)&r, sizeof( r ) );
        md5_append( &md5, (char*)&tv, sizeof( tv ) );
        md5_append( &md5, (char*)&c, sizeof( c ) ) ;
    } else {
        md5_append( &md5, (char*)&tv, sizeof( tv ) );
        md5_append( &md5, (char*)&c, sizeof( c ) ) ;
        md5_append( &md5, (char*)&r, sizeof( r ) );
    }
    md5_finish( &md5, hash );
    for( i = 0; i < 16; i ++ ){
    /*  char x[16];
      sprintf( x, "%02x-", hash[i] );
      id[i*2] = x[0]; id[i*2+1] = x[1];*/
      sprintf( id + i * 2, "%02x", hash[i] );
    }

}


/*
 * Funcion para serializar la sesion
 * */
static int  session_to_bin( Session* s, void** data ){
    int  size;
    if( binary_pack( "bill", data, &size, s->id, sizeof( s->id ), s->user_id, s->created_at, s->last_seen_at ) ){
        return size;
    } else return 0;
}

static Session*  bin_to_session( void* data, int size ){
    char* id;
    unsigned int user_id;
    time_t  created_at;
    time_t  last_seen_at;
    if( binary_unpack( "bill", data, size, &id, NULL, &user_id, &created_at, &last_seen_at ) ){
        Session * s = session_new( NULL );
        memcpy( s->id, id, 32 );
        s->user_id = user_id;
        s->created_at = created_at;
        s->last_seen_at = last_seen_at;
        return s;
    } else return NULL;
}



/*
 * Genera una nueva sesion
 * */
Session*  session_new( User* user ){
    Session* ret = malloc( sizeof( Session ) );
    memset( ret, 0, sizeof( Session ) );
    if( user ){
        ret->user = user;
        ret->user_id = user->id;
    }
    return  ret;
}


void      session_free( Session* s ){
    if( s->user && ( s->flags & SESSION_FLAGUSERID ) ) user_free( s->user );
    free( s );
}

static const char*  session_id( Session* s ){
    static  char ret[33];
    ret[32] = 0;
    memcpy( ret, s->id, 32 );
    return (const char*) ret;
}

/*
 * Salva la sesion en la base de datos
 * */
int      session_save( Session* s ){
    if( s->id[0] == 0 ){
        session_generar_id( s->id );
    }
    struct timeval tv;
    gettimeofday( &tv, NULL );
    if( !s->created_at ) s->created_at = tv.tv_sec;
    s->last_seen_at = tv.tv_sec;

    void* data;
    int size = session_to_bin( s, &data );
    if( size ){
        if( !dbput_data( DBSESSION, s->id, 32, data, size ) ){
            LOGPRINT( 1, "Error salvando session %s", dbget_lasterror() );
            return 0;
        }
    } else {
        LOGPRINT( 1, "Error fatal al pasar a binario sesion", 0 );
        return 0;
    } 
    LOGPRINT( 5, "Sesion %s salvada", session_id( s ) );
    free( data );
    return 1;
}



/*
 * Lee la sesion de la base de datos
 * */
Session*  session_load( char id[32] ){
    void * data; int size;
    int ret = dbget_data( DBSESSION, id, 32, &data, &size );
    if( !ret ){
        char s[33]; s[32] = 0;
        memcpy( id, s, 32 );
        LOGPRINT( 5, "Session con ID = %s no encontrada", s );
        return NULL;
    } else if( ret == -1 ){
        LOGPRINT( 1, "Error fatal %s", dbget_lasterror() );
        return NULL;
    }

    Session* ss = bin_to_session( data, size );
    if( !ss ){ 
        LOGPRINT( 1, "Error fatal al pasar binario a sesion", 0 );
        return NULL;
    }
    return ss;
    
}


/*
 * Devuelve el usuario de la sesion
 * */
User*     session_user( Session* s ){
    if( s->user ) return s->user;
    if( s->user_id ){
        s->user = user_load( s->user_id );
        s->flags |= SESSION_FLAGUSERID;
        return s->user;
    }
    return NULL;
}

/*
 * Sesion vencida
 * */

int       session_defeated( Session* s ){
    struct timeval tv;
    gettimeofday( &tv, NULL );
    return( s->last_seen_at + SESSION_TIMEOUT < tv.tv_sec );
    
}

