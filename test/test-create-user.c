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
#include  <stdlib.h>
#include  <unistd.h>
#include  <assert.h>

#include  "dbmanager.h"
#include  "users.h"
#include  "log.h"

#define   FILEDB  "test.db"

int  main( int argc, char** argv ){

    loglevel = 2;
    User* u;

    unlink( FILEDB );
    assert( dbset_file( FILEDB ) ) ;
    assert( init_db( FILEDB ) );
    assert( 2 == dbget_usernextid() ) ;
    dbact_close();

    unlink( FILEDB );
    assert( dbset_file( FILEDB ) ) ;
    assert( init_db( FILEDB ) );
    dbact_close();

    assert( system( "../src/qgserver-tool -d " FILEDB " -u user -n user << EOF\nhola\nEOF\n" ) == 0 );

    assert( u = user_find_by_code( "user" ) );
    assert( strcmp( u->nombre, "user" ) == 0 );
    assert( u->id == 2 );

    exit( EXIT_SUCCESS );
}
