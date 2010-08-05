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

#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>
#include  <config.h>
#include  <errno.h>
#include  <db.h>

#include  "log.h"
#include  "users.h"

static char* db_file = NULL;
static char* db_error = NULL;
static DB*   db_users = NULL;
static DB*   db_users_code = NULL;
static DB*   db_games = NULL;
static DB*   db_games_code = NULL;
static DB*   db_game_types = NULL;
static DB*   db_stats = NULL;

typedef  struct  StrStats {
    unsigned int  user_id;
    unsigned int  game_type_id;
} Stats;

#define   USERROOT "root"
#define   USERPWD  "root"

static  int  open_dbs();




int   user_getcode( DB* secdb, const DBT* pkey,const  DBT* pdata, DBT* skey ){
    memset( skey, 0, sizeof( DBT ) );
    char* code;
    int  size ;
    if( userbin_get_code( pdata->data, &code, &size ) ){
        skey->data = code;
        skey->size = size;
        return 0;
    } else {
        LOGPRINT( 1, "Fatal error !!! %s", "userbin" );
        return DB_DONOTINDEX;
    }
}
    
/*
 * Inicializa estadisticas
 * */
static  int  init_stats( DB* db ){
    DBT  key;
    uint32_t  keyval = 1;
    struct  StrStats stats;
    DBT  val;

    memset( &key, 0, sizeof( key ) );
    memset( &val, 0, sizeof( val ) );
    memset( &stats, 0, sizeof( stats ) );

    key.data = &keyval;
    key.size = sizeof( keyval );
    val.data = &stats;
    val.size = sizeof( stats );

    return db->put( db, NULL, &key, &val, DB_NOOVERWRITE );
}

/*
 * Obtiene el proximo numero de Usuario
 * */
unsigned int  dbget_usernextid( ){
    open_dbs();
    DBT  key;
    
    uint32_t  keyval = 1;
    struct  StrStats* stats;
    DBT  val;
    int  ret;

    memset( &key, 0, sizeof( key ) );
    memset( &val, 0, sizeof( val ) );

    key.data = &keyval;
    key.size = sizeof( keyval );
    ret = db_stats->get( db_stats, NULL, &key, &val, 0 );
    if( ret != 0 ){
        LOGPRINT( 1, "Error leyendo stats %d", ret );
        db_error = "Error leyendo stats";
        return 0;
    }
    stats = (struct StrStats*)val.data;
    stats->user_id ++;
    ret = db_stats->put( db_stats, NULL, &key, &val, 0 );
    if( ret != 0 ){
        LOGPRINT( 1, "Error leyendo stats %d", ret );
        db_error = "Error leyendo stats";
        return 0;
    }
    return  stats->user_id;
    
}

/*
 * Gran complejidad para esta funcion fundamental
 * */
char*  db_getlasterror( ){ return db_error; }

/*
 *
 * Esta funcion abre las bases de datos 
 *
 * */
static  int  open_dbs(){

    if( db_users ) return 1;
    int ret;
    int flags;

    LOGPRINT( 5, "Abriendo bases %s", db_file );

    // creo los espacios de memoria necesarios
    ret = db_create( &db_users, NULL, 0 );
    if( ret != 0 ){
        LOGPRINT( 2, "Error alocando %s", db_file );
        db_error = "Error alocando archivo";
        return 0;
    }
    ret = db_create( &db_users_code, NULL, 0 );
    if( ret != 0 ){
        LOGPRINT( 2, "Error alocando %s", db_file );
        db_error = "Error alocando archivo";
        return 0;
    }
    ret = db_create( &db_games, NULL, 0 );
    if( ret != 0 ){
        LOGPRINT( 2, "Error alocando %s", db_file );
        db_error = "Error alocando archivo";
        return 0;
    }
    ret = db_create( &db_games_code, NULL, 0 );
    if( ret != 0 ){
        LOGPRINT( 2, "Error alocando %s", db_file );
        db_error = "Error alocando archivo";
        return 0;
    }
    ret = db_create( &db_stats, NULL, 0 );
    if( ret != 0 ){
        LOGPRINT( 2, "Error alocando %s", db_file );
        db_error = "Error alocando archivo";
        return 0;
    }

    // Ahora abro las bases de datos
    flags = 0;
    ret = db_users->open( db_users, NULL, db_file, "users", DB_BTREE, flags, 0 );
    if( ret != 0 ){
        LOGPRINT( 2, "Error abriendo %s users", db_file );
        db_error = "Error abriendo users";
        return 0;
    }
    ret = db_games->open( db_games, NULL, db_file, "games", DB_BTREE, flags, 0 );
    if( ret != 0 ){
        LOGPRINT( 2, "Error abriendo %s games", db_file );
        db_error = "Error abriendo games";
        return 0;
    }
    ret = db_stats->open( db_stats, NULL, db_file, "stats", DB_BTREE, flags, 0 );
    if( ret != 0 ){
        LOGPRINT( 2, "Error abriendo %s stats", db_file );
        db_error = "Error abriendo stats";
        return 0;
    }

    // Este es el indice por codigo de usuario
    ret = db_users_code->open( db_users_code, NULL, db_file, "users_code_idx", 
                    DB_BTREE, DB_CREATE, 0 );
    if( ret != 0 ){
        LOGPRINT( 2, "Error abriendo %s users_code", db_file );
        db_error = "Error abriendo stats";
        return 0;
    }
    ret = db_users->associate( db_users, NULL, db_users_code, user_getcode, DB_CREATE );
    if( ret != 0 ){
        LOGPRINT( 2, "Error estableciendo asociacion users_code (%s)", db_file );
        db_error = "Error estableciendo asociacion";
        return 0;
    }
    


    return 1;


}


/*
 * Setea el archivo de datos
 * */
int    dbset_file( char* filename ){
    if( db_users ){
        LOGPRINT( 2, "La base de datos ya esta abierta (actual %s, setting %s)", db_file, filename );
        return 0;
    } else {
        db_file = filename;
    }
    return 1;
}



/*
 * Inicializa la base de datos 
 * */
int    init_db( char* filename ){
    DB* db;

    LOGPRINT( 5, "Inicializando %s", filename );
    dbset_file( filename );

    int ret = db_create( &db, NULL, 0 );
    if( ret != 0 ){
        LOGPRINT( 2, "Error alocando %s", filename );
        db_error = "Error alocando archivo";
        return 0;
    }

    int flags = DB_CREATE | DB_EXCL ;
    ret = db->open( db, NULL, filename, "users", DB_BTREE, flags, 0 );
    if( ret != 0 ){
        LOGPRINT( 2, "Error abriendo %s", filename );
        db_error = "Error abriendo archivo";
        return 0;
    }
    db->close(db,0);

    if( db_create( &db, NULL, 0 ) != 0 ){
        LOGPRINT( 2, "Error alocando %s", filename );
        db_error = "Error alocando archivo";
        return 0;
    }
    flags = DB_CREATE  ;
    ret = db->open( db, NULL, filename, "games", DB_BTREE, flags, 0 );
    if( ret != 0 ){
        LOGPRINT( 2, "Error abriendo %s (games)", filename );
        db_error = "Error abriendo archivo (games)";
        return 0;
    }
    db->close(db,0);

    flags = DB_CREATE  ;
    ret = db->open( db, NULL, filename, "game_types", DB_BTREE, flags, 0 );
    if( ret != 0 ){
        LOGPRINT( 2, "Error abriendo %s (game_types)", filename );
        db_error = "Error abriendo archivo (game_types)";
        return 0;
    }
    db->close(db,0);

    if( db_create( &db, NULL, 0 ) != 0 ){
        LOGPRINT( 2, "Error alocando %s", filename );
        db_error = "Error alocando archivo";
        return 0;
    }
    flags = DB_CREATE  ;
    ret = db->open( db, NULL, filename, "stats", DB_BTREE, flags, 0 );
    if( ret != 0 ){
        LOGPRINT( 2, "Error abriendo %s (stats)", filename );
        db_error = "Error abriendo archivo (stats)";
        return 0;
    }
    
    ret = init_stats( db );
    if( ret != 0 ){
        LOGPRINT( 2, "Error inicializando estadisticas %d", ret );
        db_error = "Error inicializando stats";
        return 0;
    };
    db->close(db,0);

    
    /* Listo el pollo, ahora hay que crear un nuevo usuario, el 
     * root */
    User* u = user_new( USERTYPE_ADMIN, USERROOT, USERROOT, USERPWD );
    if( !user_save( u ) ){
        LOGPRINT( 2, "Error al salvar usuario %s", USERROOT );
        db_error = "Error grabando usuario " USERROOT;
        return 0;
    }
    

    return 1;
}

/*
 * Graba un usuario.
 * Si esta informado el id, entonces es una actualizacion. 
 * */

int    dbput_user( unsigned int id, void* data, int size ){
    int  flags;
    DBT  key;
    DBT  val;
    uint32_t  keyval;

    if( !open_dbs() ) return 0;

    flags = 0;
    keyval = id;

    memset( &key, 0, sizeof( key ) );
    memset( &val, 0, sizeof( val ) );

    key.data = &keyval;
    key.size = sizeof( keyval );

    val.data = data;
    val.size = size;


    int  ret = db_users->put( db_users, NULL, &key, &val, flags );
    if( ret == DB_KEYEXIST || ret == EINVAL ){
        LOGPRINT( 2, "Error, clave duplicada %s", "users" );
        db_error = "Clave duplicada";
        return 0;
    } else if ( ret != 0 ){
        LOGPRINT( 1, "Error %d usuario => %d", ret, keyval );
        db_error = "Error get / put user";
        return 0;
    } else {
        LOGPRINT( 5, "Usuario %d salvado", keyval );
        return 1;
    }
}

/*
 * Obtiene los datos de un usuario a traves del indice secundario
 * que no es otra cosa que el codigo
 * */
int    dbget_user_code( char* code, void** data, int* size ){
    int  flags;
    DBT  key;
    DBT  val;
    uint32_t  keyval;
    
    if( !open_dbs() ) return 0;

    memset( &key, 0, sizeof( key ) );
    memset( &val, 0, sizeof( val ) );

    key.data = code;
    key.size = strlen(code);
    
    int ret = db_users_code->get( db_users_code, NULL, &key, &val, 0 );
    if( ret == 0 ){
        if( data ) *data = val.data;
        if( size ) *size = val.size;
        return 1;
    } else if( ret == DB_NOTFOUND ) {
        LOGPRINT( 5, "%s no encontrado", code );
        return 0;
    } else {
        LOGPRINT( 1, "Error %d al intentar obtener usuario", ret );
        db_error = "Error al intentar obtener usuario";
        return 0;
    }

    
}

/*
 * Esta funcion es muy importante, ya que es la encargada de cerrar todas
 * las bases. No hay que olvidarse de hacerlo!
 * */
void   dbact_close(){
    if( db_users )      db_users->close( db_users, 0 );
    if( db_users_code ) db_users_code->close( db_users_code, 0 );
    if( db_games )      db_games->close( db_games, 0 );
    if( db_game_types ) db_game_types->close( db_game_types, 0 );
    if( db_games_code ) db_games_code->close( db_games_code, 0 );
    if( db_stats  )     db_stats->close( db_stats, 0 );

    db_users = NULL;
    db_users_code = NULL;
    db_games = NULL;
    db_games_code = NULL;
    db_stats = NULL;
}

/*
 * Funcion para hacer un flush de los archivos
 * */
void   dbact_sync(){
    if( db_users )      db_users->sync( db_users, 0 );
    if( db_users_code ) db_users_code->sync( db_users_code, 0 );
    if( db_games )      db_games->sync( db_games, 0 );
    if( db_game_types ) db_game_types->sync( db_game_types, 0 );
    if( db_games_code ) db_games_code->sync( db_games_code, 0 );
    if( db_stats  )     db_stats->sync( db_stats, 0 );
}

/*
 * Estadisticas
 * */
void   dbget_stat( ){
    if( db_users ){
        db_users->stat_print( db_users, DB_FAST_STAT );
        db_games->stat_print( db_games, DB_FAST_STAT );
        db_stats->stat_print( db_stats, DB_FAST_STAT );
    } else {
        LOGPRINT( 5, "Bases no abiertas ", 0 );
    }
}

