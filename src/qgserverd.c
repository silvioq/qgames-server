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

#include  <stdlib.h>
#include  <unistd.h>
#include  <signal.h>
#include  "log.h"
#include  "dbmanager.h"

int   init_webservice( int port, int max_threads );
static void  trap(int a){
    LOGPRINT( 5, "Cerrando aplicacion %d", a );
    dbact_sync( );
    dbact_close( );
    exit( 0 );
}

void usage(char* prg){

    puts( "Uso:" );
    printf( "  %s [-vh] [-p port] [-w worker_threads (experimental)] [-d database_file]\n", prg );
}

int main( int argc, char** argv ){

    loglevel = 2;
    int opt;
    int port = 8080;
    int maxt = 1;
    char* db = "qgserver.db";

    while(( opt = getopt( argc, argv, "hp:vd:w:" )) != -1 ){
        switch(opt){
            case 'd':
                db = optarg;
                break;
            case 'w':
                maxt = atoi(optarg);
                if( !maxt ){
                    usage( argv[0] );
                    exit( EXIT_FAILURE );
                }
                break;
            case 'v':
                loglevel = 5;
                break;
            case 'p':
                port = atoi( optarg );
                if( !port ){
                    usage( argv[0] );
                    exit( EXIT_FAILURE );
                }
                break;
            case 'h':
                usage(argv[0]);
                exit( EXIT_SUCCESS );
                break;
            default:
                usage(argv[0]);
                exit( EXIT_FAILURE );
                break;

        }
    }

    dbset_file( db );
    if( init_webservice( port, maxt ) ){

        signal( SIGTERM, trap );
        signal( SIGINT, trap );

        while( 1 ){ sleep( 10 ); }
        exit( EXIT_SUCCESS );
    } else exit( EXIT_FAILURE );
}
