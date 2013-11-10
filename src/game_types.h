/****************************************************************************
 * Copyright (c) 2009-2013 Silvio Quadri                                    *
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

#ifndef  GAMETYPES_H
#define  GAMETYPES_H

#include <qgames.h>

typedef  struct  StrGameType {
  unsigned  int     id;
  char*             nombre;
  time_t            created_at;

  Tipojuego*        tipojuego;
  int               rec_flags;
} GameType;

GameType*  game_type_by_name( char* name );
GameType*  game_type_new( char* name, time_t created_at );
void       game_type_free( GameType* gt );
GameType*  game_type_share_by_name( char* name );
GameType*  game_type_share_by_id( unsigned int id, GameType* game_type_loaded );
void       game_type_share_clean( );
Tipojuego*  game_type_tipojuego( GameType* gt );


/*
 * Uso del cursor:
 *
 *    void * cursor = NULL; // inicializar en nulo
 *    GameType* gt; 
 *    while( game_type_next( &cursor, &gt ) ){
 *      // do something ...
 *    }
 *    game_type_end( &cursor );
 * */
int        game_type_next( void** cursor, GameType** gt );
int        game_type_end( void** cursor );

/*
 * Esta funcion recorre el directorio de definicion de tipos
 * de juego y carga en la base todos los juegos que hay disponibles
 * */
int        game_type_discover( );

int        game_type_save( GameType* gt );
GameType*  game_type_load( unsigned int id );

#endif
// vim: set cin sw=4:
