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

#include  <sys/types.h>
#include  <sys/stat.h>
#include  <stdio.h>
#include  <string.h>
#include  <stdlib.h>
#include  <unistd.h>
#include  <assert.h>

#include  "users.h"
#include  "session.h"
#include  "dbmanager.h"
#include  "log.h"

#define   FILEDB  "test.db"

int  main( int argc, char** argv ){

    loglevel = 2;
    Session* s;
    User* u;
    char id[32];
    int i;
    
    unlink( FILEDB );
    assert( dbset_file( FILEDB, NULL ) ) ;
    assert( init_db( FILEDB ) );
    dbact_close();

    assert( u = user_find_by_code( "root" ) );
    assert( s = session_new( u ) );
    assert( s->user_id == 1 );
    assert( session_save( s ) );
    memcpy( id, s->id, 32 );
    for( i = 0; i < 32; i ++ ){
        assert( ( id[i] >= '0' && id[i] <= '9' ) || ( id[i] >= 'a' && id[i] <= 'f' ) );
    }
    session_free( s );

    assert( s = session_load( id ) );
    assert( s->user_id == 1 );
    for( i = 0; i < 32; i ++ ){
        assert( (int)(id[i]) == (int)(s->id[i]) );
    }
    session_free( s );

    assert( s = session_load( id ) );
    assert( s->user_id == 1 );
    for( i = 0; i < 32; i ++ ){
        assert( id[i] == s->id[i] );
    }
    

    dbact_close();
    exit( EXIT_SUCCESS );
}
