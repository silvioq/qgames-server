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

#include  "users.h"
#include  "dbmanager.h"

void  usage(char* program){
    puts( "Uso:" );
    printf( "  %s [-c \"db to create\"] [-u user -d dbfile] -s secret_key\n" );
    exit( EXIT_FAILURE );
}


int  main( int argc, char** argv ){

    char* dbfilec = NULL;
    char* dbfile  = NULL;
    char* usercode = NULL;
    char* secret_key = NULL;

    int opt = 0;
    while(( opt = getopt( argc, argv, "hd:c:u:s:" )) != -1 ){
        switch(opt){
            case 'c':
                dbfilec = optarg;
                break;
            case 'u':
                usercode = optarg;
                break;
            case 'd':
                dbfile  = optarg;
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
          dbfile = dbfilec;
    }

    if( dbfile ){
          dbset_file( dbfile );
    }

    

    return 0;
}
