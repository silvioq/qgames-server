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

    int csize = ( u->code ? strlen( u->code ) : 0 ) + 1;
    int nsize = ( u->nombre ? strlen( u->nombre ) : 0 ) + 1;

    int size = sizeof( uint32_t ) // ID
            +  sizeof( uint8_t ) // tipo
            +  csize  // codigo
            +  nsize  // nombre
            +  sizeof( u->password ) ;

    void*  ret = malloc( size );
    void*  point = ret;
    
    // ID
    uint32_t  len32 = u->id;
    *(uint32_t*)point = len32;
    point = (char*) point + sizeof( len32 );

    // tipo
    uint8_t   len8 = u->tipo;
    *(uint8_t*)point = len8;
    point = (char*)point + sizeof( len8 );

    // codigo
    if( u->code ){
        memcpy( point, u->code, csize );
        point = (char*)point + csize ;
    } else {
        ((char*)point)[0] = 0;
        point = (char*)point + 1;
    }

    // codigo
    if( u->nombre ){
        memcpy( point, u->nombre, nsize );
        point = (char*)point + nsize ;
    } else {
        ((char*)point)[0] = 0;
        point = (char*)point + 1;
    }

    // password
    memcpy( point, u->password, sizeof( u->password ) );

    *data = ret;
    return size;
    
    
}

/*
 * Esta funcion hace precisamente lo contrario a la anterior 
 * Toma un binario y lo convierte en una estructura valida 
 * de usuario
 * */
User*  bin_to_user( void* data, int size ){

    char* max = (char*)data + size;
    char* point = data;


    char* code; char* nombre;
    unsigned int  id;
    int  tipo;

    if( point > max ) return NULL;
    tipo = (int)(((uint8_t*)point)[0]);
    point += sizeof( uint8_t );

    if( point > max ) return NULL;
    id = (unsigned int)(((uint32_t*)point)[0]);
    point += sizeof( uint32_t );

    if( point > max ) return NULL;
    code = point;
    point += strlen(code) + 1;

    if( point > max ) return NULL;
    nombre = point;
    point += strlen(nombre) + 1;

    if( point + 16 > max ) return NULL;

    User* u = user_new( tipo, code, nombre, NULL );
    u->id = id;
    memcpy( u->password, point, 16 );

    return u;

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
    size = user_to_bin( user, &data );
    if( size == 0 ){
        LOGPRINT( 1, "Error en dump de usuario %d", user->id );
        return 0;
    }
    int ret = dbput_user( user->id, data, size );
    if( ret == 0 ){
        LOGPRINT( 1, "Error salvando usuario %d (%s)", user->id, db_getlasterror() );
        return 0;
    }

    if( user->id == 0 ) user->id = ret;
    return ret;
}
