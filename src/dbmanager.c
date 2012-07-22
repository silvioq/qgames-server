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
#include  <sys/stat.h>
#include  <errno.h>
#include  <db.h>
#include  <pthread.h>

#include  "log.h"
#include  "users.h"
#include  "dbmanager.h"

static char* db_file = NULL;
static char* db_home = NULL;
static char* db_error = NULL;
static DB*   db_users = NULL;
static DB*   db_users_code = NULL;
static DB*   db_games = NULL;
static DB*   db_game_types = NULL;
static DB*   db_game_types_name = NULL;
static DB*   db_stats = NULL;
static DB*   db_sess  = NULL;
static DB_ENV* db_env  = NULL;

static  pthread_mutex_t   update_semaphore = PTHREAD_MUTEX_INITIALIZER;

typedef  struct  StrStats {
    unsigned int  user_id;
    unsigned int  game_type_id;
} Stats;

#define   USERROOT "root"
#define   USERPWD  "root"

static  int  open_dbs();




static int   user_getcode( DB* secdb, const DBT* pkey,const  DBT* pdata, DBT* skey ){
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

static int   gametype_getname( DB* secdb, const DBT* pkey,const  DBT* pdata, DBT* skey ){
    memset( skey, 0, sizeof( DBT ) );
    char* name;
    int  size ;
    if( binary_unpack( "is", pdata->data, pdata->size, NULL, &name ) ){
        skey->data = name;
        skey->size = strlen( name );
        return 0;
    } else {
        LOGPRINT( 1, "Fatal error !!! %s", "gametype_bin" );
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
    int ret;

    memset( &key, 0, sizeof( key ) );
    memset( &val, 0, sizeof( val ) );
    memset( &stats, 0, sizeof( stats ) );

    key.data = &keyval;
    key.size = sizeof( keyval );
    val.data = &stats;
    val.size = sizeof( stats );

    if( ret = db->put( db, NULL, &key, &val, DB_NOOVERWRITE ) ){
        LOGPRINT( 1, "Error inicializando stats %d %s", ret, db_strerror( ret ) );
        return 0;
    }

    // Inicializo ahora el numero de version
    keyval = 2;
    int  version = QGS_MAJOR_VERSION * 10000 + QGS_MINOR_VERSION * 100 + QGS_REV_VERSION ;
    memset( &key, 0, sizeof( key ) );
    memset( &val, 0, sizeof( val ) );
    key.data = &keyval;
    key.size = sizeof( keyval );
    val.data = &version;
    val.size = sizeof( version );

    if( ret = db->put( db, NULL, &key, &val, DB_NOOVERWRITE ) ){
        LOGPRINT( 1, "Error inicializando version %d: %s", ret, db_strerror( ret ) );
        return 0;
    }
    return 1;
}

/*
 * Obtiene las estadisticas
 *
 * */
static int dbget_stats_struct( struct  StrStats** stats ){
    open_dbs();
    DBT  key;
    
    uint32_t  keyval = 1;
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
    *stats = (struct StrStats*)val.data;
    LOGPRINT( 6, "Stats leidos user_id = %d", (*stats)->user_id );
    return 1;
}

/*
 * Graba las estadisticas
 * */
static int dbput_stats_struct( struct  StrStats* stats ){
    open_dbs();
    DBT  key;
    
    uint32_t  keyval = 1;
    DBT  val;
    int  ret;

    memset( &key, 0, sizeof( key ) );
    memset( &val, 0, sizeof( val ) );

    key.data = &keyval;
    key.size = sizeof( keyval );
    val.data = stats;
    val.size = sizeof( stats );
    if( ret = db_stats->put( db_stats, NULL, &key, &val, 0 ) ){
        LOGPRINT(1, "Error grabando stats %d %s", ret, db_strerror( ret ) );
        db_error = "Error grabando stats";
        return 0;
    }
    return 1;
}

/*
 * Devuelve el numero de version de la base de datos
 * para futuros controles
 * */
int  dbget_version(  ){
    open_dbs();
    DBT  key;
    
    uint32_t  keyval = 2;
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
    ret = *(int*)val.data;
    return ret;
}
    

/*
 * Obtiene el proximo numero de Usuario
 * */
unsigned int  dbget_usernextid( ){
    struct  StrStats* stats;
    unsigned int ret;
    if( !dbget_stats_struct( &stats ) ) return 0;
    ret = ++stats->user_id;
    if( !dbput_stats_struct( stats ) ) return 0;
    return  ret;
}

/*
 * Obtiene el proximo numero de Game Type
 * */
unsigned int  dbget_game_typenextid( ){
    struct  StrStats* stats;
    unsigned int ret;
    if( !dbget_stats_struct( &stats ) ) return 0;
    ret = ++stats->game_type_id;
    if( !dbput_stats_struct( stats ) ) return 0;
    return  ret;
}

/*
 * Gran complejidad para esta funcion fundamental
 * */
char*  dbget_lasterror( ){ return db_error; }

/*
 *
 * Esta funcion abre las bases de datos 
 *
 * */
static  int  open_dbs(){

    if( db_users ) return 1;
    pthread_mutex_lock( &update_semaphore );
    if( db_users ){
        pthread_mutex_unlock( &update_semaphore );
        return 1;
    }
    int ret;
    int flags;

    LOGPRINT( 5, "Abriendo bases %s", db_file );

    if( db_home ){
        LOGPRINT( 5, "Environment home %s", db_home );
        ret = db_env_create( &db_env, 0 );
        if( ret != 0 ){
            LOGPRINT( 2, "Error alocando env %s", db_home );
            pthread_mutex_unlock( &update_semaphore );
            return 0;
        }
        ret = db_env->open( db_env, db_home, DB_INIT_LOCK | DB_INIT_LOG | DB_INIT_MPOOL | DB_CREATE, 
                       S_IRUSR | S_IWUSR );
        if( ret != 0 ){
            LOGPRINT( 2, "Error opening env %s %d: %s", db_home, ret, db_strerror( ret ) );
            pthread_mutex_unlock( &update_semaphore );
            return 0;
        }
    }


    // creo los espacios de memoria necesarios
    ret = db_create( &db_users, db_env, 0 );
    if( ret != 0 ){
        LOGPRINT( 2, "Error alocando %s", db_file );
        db_error = "Error alocando archivo";
        pthread_mutex_unlock( &update_semaphore );
        return 0;
    }
    ret = db_create( &db_users_code, db_env, 0 );
    if( ret != 0 ){
        LOGPRINT( 2, "Error alocando %s", db_file );
        db_error = "Error alocando archivo";
        pthread_mutex_unlock( &update_semaphore );
        return 0;
    }
    ret = db_create( &db_games, db_env, 0 );
    if( ret != 0 ){
        LOGPRINT( 2, "Error alocando %s", db_file );
        db_error = "Error alocando archivo";
        pthread_mutex_unlock( &update_semaphore );
        return 0;
    }
    ret = db_create( &db_game_types, db_env, 0 );
    if( ret != 0 ){
        LOGPRINT( 2, "Error alocando %s", db_file );
        db_error = "Error alocando archivo";
        pthread_mutex_unlock( &update_semaphore );
        return 0;
    }
    ret = db_create( &db_game_types_name, db_env, 0 );
    if( ret != 0 ){
        LOGPRINT( 2, "Error alocando %s", db_file );
        db_error = "Error alocando archivo";
        pthread_mutex_unlock( &update_semaphore );
        return 0;
    }
    ret = db_create( &db_stats, db_env, 0 );
    if( ret != 0 ){
        LOGPRINT( 2, "Error alocando %s", db_file );
        db_error = "Error alocando archivo";
        pthread_mutex_unlock( &update_semaphore );
        return 0;
    }
    ret = db_create( &db_sess, db_env, 0 );
    if( ret != 0 ){
        LOGPRINT( 2, "Error alocando %s", db_file );
        db_error = "Error alocando archivo";
        pthread_mutex_unlock( &update_semaphore );
        return 0;
    }

    // Ahora abro las bases de datos
    flags = 0;
    ret = db_users->open( db_users, NULL, db_file, "users", DB_BTREE, flags, 0 );
    if( ret != 0 ){
        LOGPRINT( 2, "Error abriendo %s users (%d - %s)", db_file, ret, db_strerror( ret ) );
        db_error = "Error abriendo users";
        pthread_mutex_unlock( &update_semaphore );
        return 0;
    }
    ret = db_games->open( db_games, NULL, db_file, "games", DB_BTREE, flags, 0 );
    if( ret != 0 ){
        LOGPRINT( 2, "Error abriendo %s games", db_file );
        db_error = "Error abriendo games";
        pthread_mutex_unlock( &update_semaphore );
        return 0;
    }
    ret = db_game_types->open( db_game_types, NULL, db_file, "game_types", DB_BTREE, flags, 0 );
    if( ret != 0 ){
        LOGPRINT( 2, "Error abriendo %s game_types", db_file );
        db_error = "Error abriendo games";
        pthread_mutex_unlock( &update_semaphore );
        return 0;
    }
    ret = db_stats->open( db_stats, NULL, db_file, "stats", DB_BTREE, flags, 0 );
    if( ret != 0 ){
        LOGPRINT( 2, "Error abriendo %s stats", db_file );
        db_error = "Error abriendo stats";
        pthread_mutex_unlock( &update_semaphore );
        return 0;
    }
    ret = db_sess->open( db_sess, NULL, db_file, "sess", DB_BTREE, flags, 0 );
    if( ret != 0 ){
        LOGPRINT( 2, "Error abriendo %s sess", db_file );
        db_error = "Error abriendo sess";
        pthread_mutex_unlock( &update_semaphore );
        return 0;
    }

    // Este es el indice por codigo de usuario
    ret = db_users_code->open( db_users_code, NULL, db_file, "users_code_idx", 
                    DB_BTREE, DB_CREATE, 0 );
    if( ret != 0 ){
        LOGPRINT( 2, "Error abriendo %s users_code", db_file );
        db_error = "Error abriendo stats";
        pthread_mutex_unlock( &update_semaphore );
        return 0;
    }
    ret = db_users->associate( db_users, NULL, db_users_code, user_getcode, DB_CREATE );
    if( ret != 0 ){
        LOGPRINT( 2, "Error estableciendo asociacion users_code (%s)", db_file );
        db_error = "Error estableciendo asociacion";
        pthread_mutex_unlock( &update_semaphore );
        return 0;
    }

    // Este es el indice por nombre de tipo de juego
    ret = db_game_types_name->open( db_game_types_name, NULL, db_file, "game_type_name_idx",
                  DB_BTREE, DB_CREATE, 0 );
    if( ret != 0 ){
        LOGPRINT( 2, "Error abriendo %s game_type_name_idx => %d (%s)",
                    db_file, ret, db_strerror( ret ) );
        db_error = "Error abriendo game_type_name_idx";
        pthread_mutex_unlock( &update_semaphore );
        return 0;
    }
    ret = db_game_types->associate( db_game_types, NULL, db_game_types_name, gametype_getname, DB_CREATE );
    if( ret != 0 ){
        LOGPRINT( 2, "Error asociando %s game_type_name_idx => %d (%s)",
                    db_file, ret, db_strerror( ret ) );
        db_error = "Error asociando game_type_name_idx";
        pthread_mutex_unlock( &update_semaphore );
        return 0;
    }

    pthread_mutex_unlock( &update_semaphore );
    return 1;

}


/*
 * Setea el archivo de datos
 * */
int    dbset_file( char* filename, char* dbhome ){
    if( db_users ){
        LOGPRINT( 2, "La base de datos ya esta abierta (actual %s, setting %s)", db_file, filename );
        return 0;
    } else {
        db_file = filename;
        db_home = dbhome;
    }
    return 1;
}



/*
 * Inicializa la base de datos 
 * */
int    init_db( char* filename ){
    DB* db;

    LOGPRINT( 5, "Inicializando %s", filename );
    dbset_file( filename, NULL );

    pthread_mutex_lock( &update_semaphore );
    int ret = db_create( &db, NULL, 0 );
    if( ret != 0 ){
        LOGPRINT( 2, "Error alocando %s", filename );
        db_error = "Error alocando archivo";
        pthread_mutex_unlock( &update_semaphore );
        return 0;
    }

    int flags = DB_CREATE | DB_EXCL ;
    ret = db->open( db, NULL, filename, "users", DB_BTREE, flags, 0 );
    if( ret != 0 ){
        LOGPRINT( 2, "Error abriendo %s", filename );
        db_error = "Error abriendo archivo";
        pthread_mutex_unlock( &update_semaphore );
        return 0;
    }
    db->close(db,0);

    if( db_create( &db, NULL, 0 ) != 0 ){
        LOGPRINT( 2, "Error alocando %s", filename );
        db_error = "Error alocando archivo";
        pthread_mutex_unlock( &update_semaphore );
        return 0;
    }
    flags = DB_CREATE  ;
    ret = db->open( db, NULL, filename, "games", DB_BTREE, flags, 0 );
    if( ret != 0 ){
        LOGPRINT( 2, "Error abriendo %s (games)", filename );
        db_error = "Error abriendo archivo (games)";
        pthread_mutex_unlock( &update_semaphore );
        return 0;
    }
    db->close(db,0);

    if( db_create( &db, NULL, 0 ) != 0 ){
        LOGPRINT( 2, "Error alocando %s", filename );
        db_error = "Error alocando archivo";
        pthread_mutex_unlock( &update_semaphore );
        return 0;
    }
    flags = DB_CREATE  ;
    ret = db->open( db, NULL, filename, "game_types", DB_BTREE, flags, 0 );
    if( ret != 0 ){
        LOGPRINT( 2, "Error abriendo %s (game_types)", filename );
        db_error = "Error abriendo archivo (game_types)";
        pthread_mutex_unlock( &update_semaphore );
        return 0;
    }
    db->close(db,0);

    if( db_create( &db, NULL, 0 ) != 0 ){
        LOGPRINT( 2, "Error alocando %s", filename );
        db_error = "Error alocando archivo";
        pthread_mutex_unlock( &update_semaphore );
        return 0;
    }
    flags = DB_CREATE  ;
    ret = db->open( db, NULL, filename, "stats", DB_BTREE, flags, 0 );
    if( ret != 0 ){
        LOGPRINT( 2, "Error abriendo %s (stats)", filename );
        db_error = "Error abriendo archivo (stats)";
        pthread_mutex_unlock( &update_semaphore );
        return 0;
    }
    
    ret = init_stats( db );
    if( !ret ){
        LOGPRINT( 2, "Error inicializando estadisticas %d", ret );
        db_error = "Error inicializando stats";
        pthread_mutex_unlock( &update_semaphore );
        return 0;
    }
    db->close(db,0);

    if( db_create( &db, NULL, 0 ) != 0 ){
        LOGPRINT( 2, "Error alocando %s", filename );
        db_error = "Error alocando archivo";
        pthread_mutex_unlock( &update_semaphore );
        return 0;
    }
    flags = DB_CREATE  ;
    ret = db->open( db, NULL, filename, "sess", DB_BTREE, flags, 0 );
    if( ret != 0 ){
        LOGPRINT( 2, "Error abriendo %s (sess)", filename );
        db_error = "Error abriendo archivo (sess)";
        pthread_mutex_unlock( &update_semaphore );
        return 0;
    }
    db->close(db,0);
    pthread_mutex_unlock( &update_semaphore );
    
    /* Listo el pollo, ahora hay que crear un nuevo usuario, el 
     * root */
    User* u = user_new( USERTYPE_ADMIN, USERROOT, USERROOT, USERPWD );
    if( !user_save( u ) ){
        LOGPRINT( 2, "Error al salvar usuario %s", USERROOT );
        db_error = "Error grabando usuario " USERROOT;
        return 0;
    }

    // Muy importante es hacer flush sobre todo!
    dbact_sync( );
    

    return 1;
}

/*
 * Graba en las bases de acuerdo a los datos pasados como parametro
 * */
int    dbput_data( int db, void* keyp, int key_size, void* data, int data_size ){
    int  flags;
    DBT  key;
    DBT  val;
    DB*  dbp;


    if( !open_dbs() ) return 0;
    switch(db){
        case  DBUSER:
            dbp = db_users;
            break;
        case  DBGAME:
            dbp = db_games;
            break;
        case  DBGAMETYPE:
            dbp = db_game_types;
            break;
        case  DBSESSION:
            dbp = db_sess;
            break;
        default:
            LOGPRINT( 1, "Error de parametro db => %d", db );
            return 0;
    }

    pthread_mutex_lock( &update_semaphore );
    flags = 0;
    memset( &key, 0, sizeof( key ) );
    memset( &val, 0, sizeof( val ) );

    key.data = keyp;
    key.size = key_size;

    val.data = data;
    val.size = data_size;

    int  ret = dbp->put( dbp, NULL, &key, &val, flags );
    if( ret == DB_KEYEXIST || ret == EINVAL ){
        const char* dbn;
        const char* dbf;
        dbp->get_dbname( dbp, &dbf, &dbn );
        LOGPRINT( 2, "Error, clave duplicada %s => %d (%s)", dbn, ret, db_strerror( ret ) );
        db_error = "Clave duplicada";
        pthread_mutex_unlock( &update_semaphore );
        return 0;
    } else if ( ret != 0 ){
        LOGPRINT( 1, "Error %d %s", ret, db_strerror( ret ) );
        db_error = "Error get / put";
        pthread_mutex_unlock( &update_semaphore );
        return 0;
    } else {
        LOGPRINT( 6, "Dato salvado respuesta = %d", ret );
        pthread_mutex_unlock( &update_semaphore );
        return 1;
    }
}

/*
 * Borra un registro
 * */
int    dbdel_data( int db, void* keyp, int key_size ){
    int  flags;
    DBT  key;
    DB*  dbp;

    if( !open_dbs() ) return 0;
    switch(db){
        case  DBUSER:
            dbp = db_users;
            break;
        case  DBGAME:
            dbp = db_games;
            break;
        case  DBGAMETYPE:
            dbp = db_game_types;
            break;
        case  DBSESSION:
            dbp = db_sess;
            break;
        default:
            LOGPRINT( 1, "Error de parametro db => %d", db );
            return 0;
    }


    pthread_mutex_lock( &update_semaphore );
    flags = 0;
    memset( &key, 0, sizeof( key ) );

    key.data = keyp;
    key.size = key_size;

    int  ret = dbp->del( dbp, NULL, &key, flags );
    if( ret == DB_NOTFOUND ){
        return 1;
    } else if ( ret != 0 ){
        LOGPRINT( 1, "Error %d %s", ret, db_strerror( ret ) );
        db_error = "Error del";
        pthread_mutex_unlock( &update_semaphore );
        return 0;
    } else {
        LOGPRINT( 6, "Dato eliminado respuesta = %d", ret );
        pthread_mutex_unlock( &update_semaphore );
        return 1;
    }
}

/*
 * Graba en las bases de acuerdo a los datos pasados como parametro
 * */
int    dbget_data( int db, void* keyp, int key_size, void** data, int* data_size ){
    int  flags;
    DBT  key;
    DBT  val;
    DB*  dbp;


    if( !open_dbs() ) return 0;
    switch(db){
        case  DBUSER:
            dbp = db_users;
            break;
        case  DBGAME:
            dbp = db_games;
            break;
        case  DBGAMETYPE:
            dbp = db_game_types;
            break;
        case  DBSESSION:
            dbp = db_sess;
            break;
        case IDXUSERCODE:
            dbp = db_users_code;
            break;
        case IDXGAMETYPENAME:
            dbp = db_game_types_name;
            break;
        default:
            LOGPRINT( 1, "Error de parametro db => %d", db );
            return 0;
    }


    pthread_mutex_lock( &update_semaphore );
    flags = 0;
    memset( &key, 0, sizeof( key ) );
    memset( &val, 0, sizeof( val ) );

    key.data = keyp;
    key.size = key_size;

    int  ret = dbp->get( dbp, NULL, &key, &val, flags );
    if( ret == DB_NOTFOUND )  {
        pthread_mutex_unlock( &update_semaphore );
        return 0;
    } else if( ret == 0 ) {
        *data = val.data;
        *data_size = val.size;
        pthread_mutex_unlock( &update_semaphore );
        return 1;
    } else {
        const char* dbn;
        const char* dbf;
        dbp->get_dbname( dbp, &dbf, &dbn );
        LOGPRINT( 1, "Error en %s => %d (%s)", dbn, ret, db_strerror( ret ) );
        pthread_mutex_unlock( &update_semaphore );
        return -1;
    }
}
    

/*
 * Esta funcion es muy importante, ya que es la encargada de cerrar todas
 * las bases. No hay que olvidarse de hacerlo!
 * */
void   dbact_close(){
    pthread_mutex_lock( &update_semaphore );
    if( db_users )      db_users->close( db_users, 0 );
    if( db_users_code ) db_users_code->close( db_users_code, 0 );
    if( db_games )      db_games->close( db_games, 0 );
    if( db_game_types ) db_game_types->close( db_game_types, 0 );
    if( db_game_types_name ) db_game_types_name->close( db_game_types_name, 0 );
    if( db_stats  )     db_stats->close( db_stats, 0 );
    if( db_sess   )     db_sess ->close( db_sess , 0 );
    if( db_env )        db_env->close( db_env, 0 );

    db_users = NULL;
    db_users_code = NULL;
    db_games = NULL;
    db_game_types = NULL;
    db_game_types_name = NULL;
    db_stats = NULL;
    db_sess  = NULL;
    db_env   = NULL;

    pthread_mutex_unlock( &update_semaphore );
}

/*
 * Funcion para hacer un flush de los archivos
 * */
void   dbact_sync(){
    pthread_mutex_lock( &update_semaphore );
    if( db_users )      db_users->sync( db_users, 0 );
    if( db_users_code ) db_users_code->sync( db_users_code, 0 );
    if( db_games )      db_games->sync( db_games, 0 );
    if( db_game_types ) db_game_types->sync( db_game_types, 0 );
    if( db_game_types_name ) db_game_types_name->sync( db_game_types_name, 0 );
    if( db_stats  )     db_stats->sync( db_stats, 0 );
    if( db_sess   )     db_sess ->sync( db_sess , 0 );
    pthread_mutex_unlock( &update_semaphore );
}

/*
 * Funcion para hacer un flush de los archivos
 * */
void   dbact_verify(){
    pthread_mutex_lock( &update_semaphore );
    LOGPRINT( 2, "Not implemented yet %p", dbact_verify );
    pthread_mutex_unlock( &update_semaphore );
}

/*
 * Lectura secuencial de una base de datos ...
 * */
int    dbcur_get( void* cur, int accion, void** data, int* size ){
    DBC* dbc = (DBC*)cur;
    DBT  key;
    DBT  val;

    if( accion == DBCLOSE ){
        dbc->close( dbc );
        return 1;
    }
    int flags;
    switch( accion ){
        case DBNEXT:
            flags = DB_NEXT;
            break;
        case DBFIRST:
            flags = DB_FIRST;
            break;
        case DBPREV:  
            flags = DB_PREV;
            break;
        case DBLAST:
            flags = DB_LAST;
            break;
        default:
            LOGPRINT( 1, "Error de parametro accion => %d", accion );
            return -1;
    }


    memset( &key, 0, sizeof( key ) );
    memset( &val, 0, sizeof( val ) );
    int  ret = dbc->get( dbc, &key, &val, flags );
    if( ret == DB_NOTFOUND )  {
        return 0;
    } else if( ret == 0 ) {
        if( data )*data = val.data;
        if( size )*size = val.size;
        return 1;
    } else {
        LOGPRINT( 1, "Error leyendo cursor => %d (%s)", ret, db_strerror( ret ) );
        db_error = "Error leyendo cursor" ;
        return -1;
    }
}

/*
 * Apertura de cursor
 * */

void*  dbcur_new( int db ){
    DBC* dbc;
    DB*  dbp;
    if( !open_dbs() ) return 0;
    switch(db){
        case  DBUSER:
            dbp = db_users;
            break;
        case  DBGAME:
            dbp = db_games;
            break;
        case  DBGAMETYPE:
            dbp = db_game_types;
            break;
        case  DBSESSION:
            dbp = db_sess;
            break;
        case IDXUSERCODE:
            dbp = db_users_code;
            break;
        case IDXGAMETYPENAME:
            dbp = db_game_types_name;
            break;
        default:
            LOGPRINT( 1, "Error de parametro db => %d", db );
            return 0;
    }

    int ret = dbp->cursor( dbp, NULL, &dbc, 0 );
    if( ret == 0 ) return dbc;

    LOGPRINT( 1, "Error en %d (%s) creando cursor", ret, db_strerror( ret ) );
    db_error = "Error abriendo cursor";
    return NULL;
}


int    dbcur_end( void* cur ){
    DBC* dbc = (DBC*)cur;
    dbc->close( dbc );
    return 1;
    
}





/*
 * Estadisticas
 * */
void   dbget_stat( ){
    if( db_users ){
        db_users->stat_print( db_users, DB_FAST_STAT );
        db_games->stat_print( db_games, DB_FAST_STAT );
        db_stats->stat_print( db_stats, DB_FAST_STAT );
        db_sess->stat_print ( db_sess, DB_FAST_STAT );
    } else {
        LOGPRINT( 5, "Bases no abiertas ", 0 );
    }
}

