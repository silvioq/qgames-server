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
    GameType* gt, *gt2;
    int usados ;
    void* cursor = NULL;

    unlink( FILEDB );
    assert( dbset_file( FILEDB ) ) ;
    assert( init_db( FILEDB ) );
    dbact_close();

    assert( game_type_next( &cursor, &gt ) == 0 );
    assert( gt == NULL );

    gt = game_type_new( "Ajedrez", 0 );
    assert( gt->id == 0 );
    assert( game_type_save( gt ) );
    assert( gt->id == 1 );
    game_type_free( gt );
    
    gt = game_type_new( "Gomoku", 0 );
    assert( gt->id == 0 );
    assert( game_type_save( gt ) );
    assert( gt->id == 2 );
    game_type_free( gt );

    gt = game_type_new( "Pente", 0 );
    assert( gt->id == 0 );
    assert( game_type_save( gt ) );
    assert( gt->id == 3 );
    game_type_free( gt );

    usados = ( 1 << 1 ) + ( 1 << 2 ) + ( 1 << 3 ) ;
    assert( game_type_next( &cursor, &gt ) == 1 );
    assert( gt );
    usados -= 1 << gt->id ;

    assert( game_type_next( &cursor, &gt ) == 1 );
    assert( gt );
    usados -= 1 << gt->id ;
    
    assert( game_type_next( &cursor, &gt ) == 1 );
    assert( gt );
    usados -= 1 << gt->id ;

    assert( game_type_next( &cursor, &gt ) == 0 );
    assert( gt == NULL );
    assert( usados == 0 );
    game_type_end( &cursor );


    // Creo uno mas
    gt = game_type_new( "Jubilado", 0 );
    assert( gt->id == 0 );
    assert( game_type_save( gt ) );
    assert( gt->id == 4 );
    game_type_free( gt );

    cursor = NULL; usados = 0;
    while( game_type_next( &cursor, NULL ) ){
        usados ++;
    }
    game_type_end( &cursor );
    assert( usados == 4 );

    cursor = NULL;
    assert( game_type_next( &cursor, &gt ) );
    assert( game_type_next( &cursor, &gt ) );
    game_type_end( &cursor );
    
    assert( gt2 = game_type_share_by_name( gt->nombre ) );
    assert( gt2 == gt );
    

    exit( EXIT_SUCCESS );
}
