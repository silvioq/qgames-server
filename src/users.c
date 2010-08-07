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
#include  "users.h"

#include  "log.h"
#include  "md5.h"
#include  "packer.h"
#include  "dbmanager.h"

static  char*  secret_key = "YrfHs7SNt1rPjX4Vn7jI/XVXqgG/DVcqfooZbefGjTVd/btw3g8pTGWrt3GUeFY/";
static  int    secret_len = 0;
static  int    secret_used = 0;


/*
 * Esta funcion toma un char de entrada y devuelve el MD5.
 * */
static  void  password_md5( const char* password, unsigned char hash[16] ){
    md5_state_t  md5;
    md5_init( &md5 );

    if( !secret_used ){
        secret_len = strlen( secret_key );
        secret_used = 1;
    }

    md5_append( &md5, secret_key, secret_len );
    md5_append( &md5, password, strlen( password ) );
    md5_finish( &md5, hash );


}


/* Sin uso, por el momento */
#define  ADDDATA( data, len, dato, alloc ){ \
    if( len + sizeof( dato ) > alloc ){ \
        alloc = len + sizeof( dato ) + 256; \
        data = realloc( data, alloc ); \
    } \
    typeof(dato)* point = (typeof(dato)*)( ((char*)data) + len );\
    *point = dato ;\
    len += sizeof( dato ); \
  }


/*
 * Esta funcion es fundamental y lo que hace simplemente es pasar la estructura
 * de usuario a un binario, para ser almacenado en la base de datos
 * Crea el espacio en memoria (darle free luego) y devuelve la
 * cantidad de bytes de espacio.
 * */
static   int  user_to_bin( User* u, void** data ){
    int size;
    if( binary_pack( "icssb", data, &size, u->id, u->tipo, u->code, u->nombre, u->password, 16 ) ){
        return size;
    }
    return 0;
}

/*
 * Esta funcion hace precisamente lo contrario a la anterior 
 * Toma un binario y lo convierte en una estructura valida 
 * de usuario
 * */
User*  bin_to_user( void* data, int size ){

    int  id;
    int  tipo;
    char* code; char* nombre;
    char* password;
    int x;
    if( binary_unpack( "icssb", data, size, &id, &tipo, &code, &nombre, &password, (int*)NULL ) ){
        User* u = user_new( tipo, code, nombre, NULL );
        u->id = id;
        memcpy( u->password, password, 16 );
        return u;
    } else {  
        return NULL;
    }

}

/*
 * Esta funcion es especialmente util para obtener el dato
 * desde el binario 
 * */

int   userbin_get_code( void* data, char** code, int* size ){
    char* point = (char*)data  +
        sizeof( uint32_t ) + // el id
        sizeof( uint8_t ) ;  // el tipo 

    int len = strlen( point );
    *code = point;
    *size = len;
    return 1;
}







User*   user_new( int tipo, char* code, char* nombre, char* password ){
    User* u = malloc( sizeof( User ) );
    memset( u, 0, sizeof( User ) );
    u->tipo = tipo;
    u->code = code ? strdup( code ) : NULL;
    u->nombre = nombre ? strdup( nombre ) : NULL;
    if( password ) password_md5( password, u->password );
    return u ;
}


void    user_free( User* user ){
    if( user->code ) free( user->code );
    if( user->nombre ) free( user->nombre );
    free( user );
}

int     user_save( User* user ){
    void* data;
    int  size;
    if( !user->id ) user->id = dbget_usernextid();
    size = user_to_bin( user, &data );
    if( size == 0 ){
        LOGPRINT( 1, "Error en dump de usuario %d", user->id );
        return 0;
    }
    int ret;

    ret = dbput_data( DBUSER, &user->id, sizeof( user->id ), data, size );
    if( ret == 0 ){
        LOGPRINT( 5, "Error salvando usuario %d (%s)", user->id, db_getlasterror() );
        return 0;
    }

    return 1;
}


void    user_set_password( User* user, char* password ){
    if( password ) password_md5( password, user->password );
}

/* 
 * Dado un codigo de usuario, obtiene la informacion
 * */
User*   user_find_by_code( char* code ){
    void* data;
    int size;

    if( dbget_user_code( code, &data, &size ) ){
        return  bin_to_user( data, size );
    } else {
        return NULL;
    }
}
