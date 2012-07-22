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

#ifndef  DBMANAGER_H
#define  DBMANAGER_H

#define  DBUSER      1
#define  DBGAME      2
#define  DBGAMETYPE  3
#define  DBSESSION   4

#define  IDXUSERCODE  11
#define  IDXGAMETYPENAME  12


int    init_db( char* filename );
int    dbset_file( char* filename, char* dbhome );
int    dbput_data( int db, void* key, int key_size, void* data, int data_size );
int    dbget_data( int db, void* key, int key_size, void** data, int* data_size );
int    dbdel_data( int db, void* key, int key_size );
unsigned int  dbget_usernextid( );
unsigned int  dbget_game_typenextid( );
// int    dbget_game( unsigned int id, void** data, int* size );
int    dbget_user_code( char* code, void** data, int* size );
void   dbget_stat( );
void   dbact_close();
void   dbact_sync();
void   dbact_verify( );
int    dbget_version(  );
char*  dbget_lasterror( );

#define    DBPREV  -2
#define    DBLAST  -1
#define    DBFIRST  1
#define    DBNEXT   2
#define    DBCLOSE  0
void*  dbcur_new( int db );

/*
 * Retorna:
 *  1: Ok
 *  0: EOF
 *  -1: Error
 *  */
int    dbcur_get( void* cur, int accion, void** data, int* size );
int    dbcur_end( void* cur );

#endif
