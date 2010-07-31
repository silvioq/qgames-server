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
#include  "users.h"

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


User*   user_new( int tipo, char* code, char* nombre, char* password ){
    User* u = malloc( sizeof( User ) );
    memset( u, 0, sizeof( User ) );
    u->tipo = tipo;
    u->code = code ? strdup( code ) : NULL;
    u->nombre = nombre ? strdup( nombre ) : NULL;
    password_md5( password, u->password );
    return u ;
}


void    user_free( User* user ){
    if( user->code ) free( user->code );
    if( user->nombre ) free( user->nombre );
    free( user );
}

int     user_save( User* user ){
    return 0;
}
