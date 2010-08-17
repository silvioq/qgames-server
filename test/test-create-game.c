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

#include  "users.h"
#include  "games.h"
#include  "dbmanager.h"
#include  "log.h"

#define   FILEDB  "test.db"

int  main( int argc, char** argv ){

    loglevel = 2;
    GameType* gt;
    Game*     g;
    User*     u;
    char buff[100];
    int i;

    unlink( FILEDB );
    assert( dbset_file( FILEDB ) ) ;
    assert( init_db( FILEDB ) );
    dbact_close();

    assert( gt = game_type_share_by_name( "Ajedrez" ) );
    assert( gt->id == 1 );

    assert( u = user_find_by_code( "root" ) );
    assert( u->id ==  1 );

    g = game_new( "x", u, gt, 0 );

    assert( game_save( g ) );
    game_free( g );

    assert( g = game_load( "x" ) );
    assert( g->created_at > 0 );
    assert( game_game_type( g )->id == gt->id );
    assert( game_user( g )->id == u->id );


    for( i = 0; i < 100; i ++ ){
        buff[i] = i * 97 % 256;
    }
    game_set_data( g, buff, 100 );
    assert( game_save( g ) );
    game_free( g );

    assert( g = game_load( "x" ) );
    assert( g->created_at > 0 );
    assert( game_game_type( g )->id == gt->id );
    assert( game_user( g )->id == u->id );
    assert( g->data_size == 100 );
    for( i = 0; i < 100; i ++ ){
        assert( ((char*)g->data)[i] == (char)( i * 97 % 256 ) );
    }
    game_free( g );
    

    exit( EXIT_SUCCESS );
}
