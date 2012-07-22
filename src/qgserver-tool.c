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
#include  <unistd.h>
#include  <config.h>

#include  "log.h"
#include  "users.h"
#include  "dbmanager.h"

void  usage(char* program){
    puts( "Uso:" );
    printf( "  %s [-c \"db to create\"] [-u user] -p [-n username] [-d dbfile] [-H home] -s secret_key\n", program );
    exit( EXIT_FAILURE );
}


int  main( int argc, char** argv ){

    char* dbfilec = NULL;
    char* dbfile  = NULL;
    char* dbhome  = NULL;
    char* usercode = NULL;
    char* username = NULL;
    char* secret_key = NULL;
    int   con_password = 0;
    if( argc == 1 ){
        usage(argv[0]);
        exit( EXIT_FAILURE );
    }

    loglevel = 2;

    int opt = 0;
    while(( opt = getopt( argc, argv, "vhd:c:u:n:s:p" )) != -1 ){
        switch(opt){
            case 'c':
                dbfilec = optarg;
                break;
            case 'u':
                usercode = optarg;
                break;
            case 'n':
                username = optarg;
                break;
            case 'p':
                con_password = 1;
                break;
            case 'v':
                loglevel = 5;
                break;
            case 'd':
                dbfile  = optarg;
                break;
            case 'H':
                dbhome   = optarg;
                break;
            case 's':
                secret_key  = optarg;
                printf( "Secret key aun no implementado ... \n" );
                break;
            default:
                usage(argv[0]);
        }
    }

    if( dbfilec && dbfile ) usage(argv[0]);
    if( usercode && !dbfile ) usage(argv[0]);

    if( dbfilec ){
          init_db( dbfilec );
    }

    if( dbfile ){
          dbset_file( dbfile, dbhome );
    }

    if( usercode ){
        User* u = user_find_by_code( usercode );
        if( !u && !username ){
            printf( "Usuario %s no encontrado ... para crear usuario use opcion -n\n", usercode );
            dbact_close();
            exit( EXIT_FAILURE );
        } else if( !u ){
            LOGPRINT( 5, "Nuevo usuario %s %s", usercode, username );
            u = user_new( USERTYPE_USER, usercode, username, "" );
        }
        char  pwd[256];
        printf( "Password: " );
        if( scanf( "%255s", pwd ) ){
            user_set_password( u, pwd );
            user_save( u );
        }
    }


    dbact_close();

    

    return 0;
}
