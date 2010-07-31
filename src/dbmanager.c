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
#include  <db.h>

#include  "log.h"
#include  "users.h"

static char* db_file = NULL;
static char* db_error = NULL;
static DB*   db_users = NULL;
static DB*   db_users_code = NULL;
static DB*   db_games = NULL;
static DB*   db_games_code = NULL;
static DB*   db_stats = NULL;

typedef  struct  StrStats {
    int  user_id;
    int  game_id;
} Stats;

#define   USERROOT "root"
#define   USERPWD  "root"

static  int  init_stats( DB* db ){
    DBT  key;
    int  keyval = 1;
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
    flags = DB_AUTO_COMMIT;
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
    ret = db_stats->open( db_games, NULL, db_file, "stats", DB_BTREE, flags, 0 );
    if( ret != 0 ){
        LOGPRINT( 2, "Error abriendo %s stats", db_file );
        db_error = "Error abriendo stats";
        return 0;
    }


    // Y finalmente los indices duplicados
    //
    // void
    // second()
    // {
    //   DB *dbp, *sdbp;
    //   int ret;
    //   
    //   /* Open/create primary */
    //   if ((ret = db_create(&dbp, dbenv, 0)) != 0)
    //     handle_error(ret);
    //   if ((ret = dbp->open(dbp, NULL,
    //       "students.db", NULL, DB_BTREE, DB_CREATE, 0600)) != 0)
    //     handle_error(ret);
    //   
    //   /*
    //  *   * Open/create secondary.  Note that it supports duplicate data
    //  *     * items, since last names might not be unique.
    //  *       */
    //   if ((ret = db_create(&sdbp, dbenv, 0)) != 0)
    //     handle_error(ret);
    //   if ((ret = sdbp->set_flags(sdbp, DB_DUP | DB_DUPSORT)) != 0)
    //     handle_error(ret);
    //   if ((ret = sdbp->open(sdbp, NULL,
    //       "lastname.db", NULL, DB_BTREE, DB_CREATE, 0600)) != 0)
    //     handle_error(ret);
    //   
    //   /* Associate the secondary with the primary. */
    //   if ((ret = dbp->associate(dbp, NULL, sdbp, getname, 0)) != 0)
    //     handle_error(ret);
    // }
    // 
    // /*
    //  *  * getname -- extracts a secondary key (the last name) from a primary
    //  *   *  key/data pair
    //  *    */
    // int
    // getname(secondary, pkey, pdata, skey)
    //   DB *secondary;
    //   const DBT *pkey, *pdata;
    //   DBT *skey;
    // {
    //   /*
    //  *   * Since the secondary key is a simple structure member of the
    //  *     * record, we don't have to do anything fancy to return it.  If
    //  *       * we have composite keys that need to be constructed from the
    //  *         * record, rather than simply pointing into it, then the user's
    //  *           * function might need to allocate space and copy data.  In
    //  *             * this case, the DB_DBT_APPMALLOC flag should be set in the
    //  *               * secondary key DBT.
    //  *                 */
    //   memset(skey, 0, sizeof(DBT));
    //   skey->data = ((struct student_record *)pdata->data)->last_name;
    //   skey->size = sizeof((struct student_record *)pdata->data)->last_name;
    //   return (0);
    // }


    return 1;


}



/*
 * Inicializa la base de datos 
 * */
int    init_db( char* filename ){
    DB* db;
    int ret = db_create( &db, NULL, 0 );
    if( ret != 0 ){
        LOGPRINT( 2, "Error alocando %s", filename );
        db_error = "Error alocando archivo";
        return 0;
    }

    int flags = DB_CREATE | DB_AUTO_COMMIT | DB_EXCL ;
    ret = db->open( db, NULL, filename, "users", DB_BTREE, flags, 0 );
    if( ret != 0 ){
        LOGPRINT( 2, "Error abriendo %s", filename );
        db_error = "Error abriendo archivo";
        return 0;
    }
    db->close(db,0);

    flags = DB_CREATE | DB_AUTO_COMMIT  ;
    ret = db->open( db, NULL, filename, "games", DB_BTREE, flags, 0 );
    if( ret != 0 ){
        LOGPRINT( 2, "Error abriendo %s (games)", filename );
        db_error = "Error abriendo archivo (games)";
        return 0;
    }
    db->close(db,0);

    flags = DB_CREATE | DB_AUTO_COMMIT  ;
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




