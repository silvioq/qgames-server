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
#include  <qgames.h>
#include  "log.h"
#include  "dbmanager.h"
#include <sys/prctl.h>


#define    DEFAULT_PORT   "8080"
#define    DEFAULT_DB     "qgserver.db"

int   init_webservice( char* port, int max_threads );
static void  trap(int a){
    static int closing = 0;
    LOGPRINT( 4, "Cerrando aplicacion %d", a );
    if( a == SIGSEGV ){ 
        dbact_close( );
        exit( EXIT_FAILURE );
    }
    if( closing ){
        LOGPRINT( 4, "App closing", 0 );
        return;
    }
    closing = 1;
    stop_webservice( );
    LOGPRINT( 4, "Cerrando bases %d", a );
    dbact_close( );
    game_type_share_clean();
    exit( EXIT_SUCCESS );
}

void usage(char* prg){

    puts( "Uso:" );
    printf( "  %s  [options]\n" 
           "-v                                  Verbose\n"
           "-vv                                 Muy Verbose\n"
           "-p port|bindaddr:port               Puerto del servidor (" DEFAULT_PORT ")\n"
           "-w worker_threads (experimental)    Cantidad de workers (1)\n"
           "-d database_file                    Archivo de base (" DEFAULT_DB ")\n"
           "-H environment home                 Environment para la base de datos (por defecto, \n"
           "                                    no se usa dbhome para DbBerkeley)\n" 
           "-g gamepath                         Directorio de definiciones de juegos\n"
           "-i imagepath                        Directorio de imágenes\n" 
           "-D                                  Dump - no trapea Segmentation fault - no recomendado \n" 
           "-h                                  Esta pantalla      \n"
         , prg );
}

int main( int argc, char** argv ){

    loglevel = 4;
    int opt;
    int trapSF = 1;
    char* port = DEFAULT_PORT;
    int maxt = 1;
    char* db = DEFAULT_DB;
    char* path_games = NULL;
    char* path_images = NULL;
    char* dbhome = NULL;

    while(( opt = getopt( argc, argv, "hp:vd:w:g:i:H:D" )) != -1 ){
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
                loglevel = loglevel == 5 ? 6 : 5;
                break;
            case 'p':
                port = optarg;
                break;
            case 'g':
                path_games = optarg ;
                break;
            case 'i':
                path_images = optarg ;
                break;
            case 'H':
                dbhome = optarg;
                break;
            case 'D':
                trapSF = 0;
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

    dbset_file( db, dbhome );
    if( path_games ){
        LOGPRINT( 4, "Path games => %s", path_games );
        qg_path_set( path_games );
    }
    if( path_images ){
        LOGPRINT( 4, "Path images => %s", path_images );
        qgames_graph_image_dir( path_images );
    }

    if( init_webservice( port, maxt ) ){

        signal( SIGTERM, trap );
        signal( SIGINT, trap );
        if( trapSF ) signal( SIGSEGV, trap );

        while( 1 ){ sleep( 10 ); }
        exit( EXIT_SUCCESS );
    } else exit( EXIT_FAILURE );
}
