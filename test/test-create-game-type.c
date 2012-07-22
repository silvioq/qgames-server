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

void    chequear_lectura_del_archivo_de_definiciones(){
    unlink( FILEDB );
    assert( dbset_file( FILEDB, NULL ) ) ;
    assert( init_db( FILEDB ) );
    assert( game_type_discover() );
    assert( dbget_game_typenextid() > 3 ) ;
}


int  main( int argc, char** argv ){

    loglevel = 2;
    GameType* gt, *gt2;

    unlink( FILEDB );
    assert( dbset_file( FILEDB, NULL ) ) ;
    assert( init_db( FILEDB ) );
    assert( 1 == dbget_game_typenextid() ) ;
    dbact_close();

    unlink( FILEDB );
    assert( dbset_file( FILEDB, NULL ) ) ;
    assert( init_db( FILEDB ) );
    dbact_close();

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

    assert( gt = game_type_by_name( "Ajedrez" ) );
    assert( gt->id == 1 );
    game_type_free( gt );
    
    assert( !game_type_by_name( "Ajedrez2" ) );

    assert( gt = game_type_by_name( "Gomoku" ) );
    assert( gt->id == 2 );
    assert( strcmp( "Gomoku", gt->nombre ) == 0 );
    game_type_free( gt );

    // A partir de ahora, intento crear el tipo de juego
    // por el metodo normal
    dbact_close();
    unlink( FILEDB );
    assert( dbset_file( FILEDB, NULL ) ) ;
    assert( init_db( FILEDB ) );

    assert( gt = game_type_share_by_name( "Ajedrez" ) );
    assert( gt2 = game_type_share_by_name( "Ajedrez" ) );
    assert( gt == gt2 );
    assert( 2 == dbget_game_typenextid() ) ;
    assert( gt = game_type_share_by_name( "Gomoku" ) );
    assert( gt != gt2 );
    assert( gt2 = game_type_share_by_name( "Gomoku" ) );
    assert( gt == gt2 );
    assert( 4 == dbget_game_typenextid() ) ;

    // Este no lo puedo encontrar
    assert( !game_type_share_by_name( "GomokuNotFound" ) );
    assert( !game_type_share_by_id( 2, NULL ) );

    assert( game_type_share_by_id( 1, NULL ) == game_type_share_by_name( "Ajedrez" ) );
    assert( game_type_share_by_id( 3, NULL ) == gt );
    dbact_close( );

    chequear_lectura_del_archivo_de_definiciones();
    exit( EXIT_SUCCESS );
}
