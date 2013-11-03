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
#include  <string.h>
#include  <stdlib.h>
#include  <stdio.h>
#include  <sys/time.h>
#include  <sys/types.h>
#include  <sys/stat.h>
#include  <unistd.h>
#include  <time.h>
#include  <pthread.h>

#include  "mongoose.h"
#include  "users.h"
#include  "log.h"
#include  "session.h"
#include  "cJSON.h"
#include  "webserver.h"


#define  POST_BUFFER_LEN   8192
typedef  struct {
    int    size_get;
    int    size_post;
    char   buffer[1];
}  StrBuffer;



static  void    routes_filter(struct mg_connection *conn, const struct mg_request_info *ri);
static  void    free_post_data( const struct mg_request_info *ri );
static  void    read_post_data( struct mg_connection *conn, const struct mg_request_info *ri );



/*
 *
 * Estas son las funciones que permiten renderizados estandares
 *
 * */

void render_404(struct mg_connection *conn, const struct mg_request_info *ri){
    int   status = 404;
    char* reason = "Not found";
    char* buff   = "Route not found";
    int   len    = strlen( buff );
		mg_printf(conn,
		    "HTTP/1.1 %d %s\r\n"
		    "Content-Type: text/plain\r\n"
		    "Content-Length: %d\r\n"
		    "Connection: close\r\n"
		    "\r\n%s", status, reason, len, buff);
}

void render_403(struct mg_connection *conn, const struct mg_request_info *ri){
    int   status = 403;
    char* reason = "Not authorized";
    char* buff   = "Not authorized. Must login first.";
    int   len    = strlen( buff );
		mg_printf(conn,
		    "HTTP/1.1 %d %s\r\n"
		    "Content-Type: text/plain\r\n"
		    "Content-Length: %d\r\n"
		    "Connection: close\r\n"
		    "\r\n%s", status, reason, len, buff);
}

void render_400(struct mg_connection *conn, const struct mg_request_info *ri, char* buf){
    int   status = 404;
    char* reason = "Bad request";
    int   len    = strlen( buf );
		mg_printf(conn,
		    "HTTP/1.1 %d %s\r\n"
		    "Content-Type: text/plain\r\n"
		    "Content-Length: %d\r\n"
		    "Connection: close\r\n"
		    "\r\n%s", status, reason, len, buf);
}

void render_200(struct mg_connection *conn, const struct mg_request_info *ri, char* buf){
    int   status = 200;
    char* reason = "OK";
    int   len    = strlen( buf );
		mg_printf(conn,
		    "HTTP/1.1 %d %s\r\n"
		    "Content-Type: text/plain\r\n"
		    "Content-Length: %d\r\n"
		    "Connection: close\r\n"
		    "\r\n", status, reason, len);

    mg_write( conn, buf, len );
}

void render_200f(struct mg_connection *conn, const struct mg_request_info *ri, FILE* f){
    int   status = 200;
    char* reason = "OK";
    struct stat st;
    fflush( f );
    fstat( fileno( f ), &st );
    int   len    = st.st_size;
    
		mg_printf(conn,
		    "HTTP/1.1 %d %s\r\n"
		    "Content-Type: text/plain\r\n"
		    "Content-Length: %d\r\n"
		    "Connection: close\r\n"
		    "\r\n", status, reason, len);

    char buff[1024];
    rewind( f );
    while( !feof( f ) ){
        if( fgets( buff, 1024, f ) )
          mg_write( conn, buff, strlen( buff ) );
    }
}

void render_200j(struct mg_connection *conn, const struct mg_request_info *ri, cJSON* root, int f){

  char* buf = NULL;
  cJSON* t;
  switch(f){
    case FORMAT_JSON:
      buf = cJSON_Print( root );
      break;

    case FORMAT_TXT:
      t = cJSON_GetObjectItem( root, "texto" );
      if( t && t->type == cJSON_String ) buf = strdup( t->valuestring );
      if( !buf ) buf = strdup( "Sin mensaje" );
      break;

    case FORMAT_XML:
      render_500( conn, ri, "Formato XML no soportado" );
      return;

    case FORMAT_YAML:
    default:
      buf = cJSON_Print_YAML( root );
      break;
  }
  render_200( conn, ri, buf );
  free( buf );
}

void render_500(struct mg_connection *conn, const struct mg_request_info *ri, char* buf){
    int   status = 500;
    char* reason = "Internal Error";
    int   len    = strlen( buf );
		mg_printf(conn,
		    "HTTP/1.1 %d %s\r\n"
		    "Content-Type: text/plain\r\n"
		    "Content-Length: %d\r\n"
		    "Connection: close\r\n"
		    "\r\n%s", status, reason, len, buf);
}


static  void * event_handler (enum mg_event event,
    struct mg_connection *conn,
    const struct mg_request_info *request_info){

    switch( event ){
        case MG_NEW_REQUEST:
            LOGPRINT( 5, "Request %p from %d.%d.%d.%d:%d => %s", request_info,
                            (int)( request_info->remote_ip & 0xFF000000 ) >> 24,
                            (int)( request_info->remote_ip & 0xFF0000 ) >> 16,
                            (int)( request_info->remote_ip & 0xFF00 ) >> 8,
                            (int)( request_info->remote_ip & 0xFF ) ,
                            (int)request_info->remote_port, 
                            request_info->uri );
            routes_filter( conn, request_info );
            free_post_data( request_info );
            return (void*)1;
        case MG_EVENT_LOG:
            LOGPRINT( 2, "[mongoose event: %s]", request_info->log_message );
            break;
        case MG_REQUEST_COMPLETE:
            LOGPRINT( 5, "Request complete %p", request_info );
            break;

    }
    return  NULL;
}

/*
 * Lee la informacion POSTeada por el cliente, en el buffer temporal
 * */
static  void    read_post_data( struct mg_connection *conn, const struct mg_request_info *ri ){
    StrBuffer*  sbuff;
    if( !ri->user_data ){
        int  max_val = POST_BUFFER_LEN;
        const char* clen = mg_get_header( conn, "Content-Lenght" );
        if( clen ){
            max_val = atol( clen );
        }
        sbuff = malloc( max_val + sizeof( int ) * 2 );
        sbuff->size_post = mg_read( conn, sbuff->buffer, max_val );
        sbuff->size_get  = ( ri->query_string ? strlen( ri->query_string ) : 0 );
        ((struct mg_request_info *)ri)->user_data = sbuff;
    }
}

static  void    free_post_data( const struct mg_request_info *ri ){
   if( ri->user_data ) free( ri->user_data );
}


/*
 * Retorna el binario con los datos POSTeados
 * */
int     get_post_data( struct mg_connection *conn, const struct mg_request_info *ri, void** data, int* size ){
    StrBuffer*  sbuff;
    read_post_data( conn, ri );
    sbuff = (StrBuffer*)ri->user_data;
    if( !sbuff->size_post ) return 0;
    if( size ) *size = sbuff->size_post;
    if( data ) *data = sbuff->buffer;
    return sbuff->size_post;
}

/*
 * Retorna una variable "encoded". 
 * Primero verifica las variables GET's y luego las POST's
 * */
char*   get_var( struct mg_connection *conn, const struct mg_request_info *ri, char* var ){
    
    StrBuffer*  sbuff;
    read_post_data( conn, ri );
    sbuff = (StrBuffer*)ri->user_data;

    int   ret_size = sbuff->size_get + sbuff->size_post;
    char* ret= malloc( ret_size );

    if( ri->query_string ){
        if( mg_get_var( ri->query_string, sbuff->size_get, var, ret, ret_size ) > 0 ){
            return ret ;
        }
    }

    if( sbuff->size_post ){
        if( mg_get_var( sbuff->buffer, sbuff->size_post, var, ret, ret_size ) > 0 ){
            return  ret ;
        }
    }
    free( ret );
    return NULL;
}


static void routes_filter(struct mg_connection *conn, const struct mg_request_info *ri){
    char buff[1024];
    int ret;
    int   my_action, my_format, my_controller;
    char  my_param[100], my_session[32];

    LOGPRINT( 5, "Nueva peticion recibida => %s", ri->uri );

    
    static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_lock( &mutex );
    ret = get_ruta( ri->uri );
    my_action = route_action;
    my_format = route_format;
    my_controller = route_controller;
    strncpy( my_session, route_session, 32 );
    strncpy( my_param, route_param, 100 );
    pthread_mutex_unlock( &mutex );

    if( ret ){
        Session *s;
        if( my_session && my_session[0] ){
            s = session_load( my_session );
            if( !s || session_defeated( s ) ){
                if( s ) session_free( s );
                render_403( conn, ri );
                return ;
            }
        } else {
            s = NULL;
        }
        switch(my_controller){
            case CONTROLLER_LOGIN:
                login_controller( conn, ri, s, my_action, my_format );
                break;
            case  CONTROLLER_GAME:
                game_controller( conn, ri, s, my_action, my_format, my_param );
                break;
            case  CONTROLLER_HELP:
                help_controller( conn, ri, my_action, my_format );
                break;
            default:
                LOGPRINT( 5, "Ruta incorrecta => %s", ri->uri );
                render_404( conn, ri );
        }
        if( s ) session_free( s );
    } else {
        LOGPRINT( 5, "Ruta incorrecta => %s", ri->uri );
        render_404( conn, ri );
    }
    return;

}


static struct mg_context *ctx;

int   init_webservice( char* port, int maxthreads ){
    game_type_discover();
    char maxth[32];
    sprintf( maxth, "%d", maxthreads );

    char* options[] = { "num_threads", maxth, "p", port, NULL };
    LOGPRINT( 4, "port => %s", port );
    LOGPRINT( 4, "num_threads => %s", maxth );

    // Verifico los juegos que hay disponibles
    ctx = mg_start( event_handler, NULL, (const char**) options );
    if( !ctx ){
       LOGPRINT( 1, "No puede abrirse el webservice", 0 );
       return 0;
    }

    LOGPRINT( 5, "started %p", ctx );
    return 1;
}


int   stop_webservice( ){
    LOGPRINT( 5, "stopping %p", ctx );
    mg_stop(ctx);
    LOGPRINT( 5, "stopped", 0 );
}
