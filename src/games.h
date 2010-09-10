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

#ifndef  GAMES_H
#define  GAMES_H

#include <qgames.h>

typedef  struct  StrGameType {
  unsigned  int     id;
  char*             nombre;
  time_t            created_at;

  Tipojuego*        tipojuego;
  int               rec_flags;
} GameType;


typedef  struct  StrGame {
  char*             id;
  unsigned int      user_id;
  User*             user;
  unsigned int      game_type_id;
  GameType*         game_type;
  unsigned int      data_size;
  void*             data;
  time_t            created_at;
  time_t            modified_at;

  Partida*          partida;
  int               rec_flags;
} Game;




Game*   game_load( char* id ); 
void    game_free( Game* game );
void    game_set_data( Game*, void* data, unsigned int data_size );
User*   game_user( Game* g );

/*
 * Esta funcion devuelve uno o cero dependiendo si el usuario
 * tiene permisos o no sobre el juego.
 * En el caso que el usuario sea administrador, tiene permisos.
 * Si es un usuario comun y corriente, deberia ser el creador
 * */
int     game_check_user( Game* g, User* u );
GameType*  game_game_type( Game* g );

int     game_save( Game* game );
int     game_del( Game* game );
Game*   game_new( char* id, User* user, GameType* type, time_t created_at );


GameType*  game_type_by_name( char* name );
GameType*  game_type_new( char* name, time_t created_at );
void       game_type_free( GameType* gt );
GameType*  game_type_share_by_name( char* name );
GameType*  game_type_share_by_id( unsigned int id, GameType* game_type_loaded );


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

int        game_type_save( GameType* gt );
GameType*  game_type_load( unsigned int id );


/*
 * Estas funciones son de interaccion con el qgames 
 * */

Partida*  game_partida( Game* g );
void      game_set_partida( Game* g, Partida* p );
Game*     game_type_create( GameType* gt, User* u );




#endif
