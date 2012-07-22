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

/*
 * En este test vamos a controlar la validez de un usuario dado un 
 * juego
 * */

int  main( int argc, char** argv ){

    loglevel = 2;
    GameType* gt;
    Game*     g;
    User*     u;
    User*     u2;
    int i;

    unlink( FILEDB );
    assert( dbset_file( FILEDB, NULL ) ) ;
    assert( init_db( FILEDB ) );
    dbact_close();

    assert( gt = game_type_share_by_name( "Ajedrez" ) );
    assert( gt->id == 1 );

    assert( u = user_find_by_code( "root" ) );
    assert( u->id ==  1 );

    // Creo el usuario 2
    assert( u2 = user_new( USERTYPE_USER, "test", "test", "test" ) );
    assert( user_save( u2 ) );
    assert( u2->id ==  2 );

    g = game_new( "x1", u, gt, 0 );
    assert( game_save( g ) );
    game_free( g );

    g = game_new( "x2", u2, gt, 0 );
    assert( game_save( g ) );
    game_free( g );

    assert( g = game_load( "x1" ) );
    assert( g->created_at > 0 );
    assert( game_game_type( g )->id == gt->id );
    assert( game_user( g )->id == u->id );
    assert( game_check_user( g, u ) );
    assert( !game_check_user( g, u2 ) );
    game_free( g );

    
    assert( g = game_load( "x2" ) );
    assert( g->created_at > 0 );
    assert( game_game_type( g )->id == gt->id );
    assert( game_user( g )->id != u->id );
    assert( game_user( g )->id == u2->id );
    assert( game_check_user( g, u ) );
    assert( game_check_user( g, u2 ) );
    game_free( g );

    exit( EXIT_SUCCESS );
}
