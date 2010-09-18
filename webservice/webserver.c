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
#include  <stdio.h>
#include  <sys/time.h>
#include  <sys/types.h>
#include  <sys/stat.h>
#include  <unistd.h>
#include  <time.h>
#include  "mongoose.h"
#include  "users.h"
#include  "log.h"
#include  "session.h"
#include  "webserver.h"





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
		    "\r\n%s", status, reason, len, buf);
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
        fgets( buff, 1024, f );
        mg_printf( conn, buff );
    }
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





static void routes_filter(struct mg_connection *conn, const struct mg_request_info *ri, void *data){
    char buff[1024];
    int ret;

    LOGPRINT( 5, "Nueva peticion recibida => %s", ri->uri );

    if( get_ruta( ri->uri ) ){
        Session *s;
        if( route_session[0] ){
            s = session_load( route_session );
            if( !s || session_defeated( s ) ){
                render_403( conn, ri );
                return ;
            }
        } else {
            s = NULL;
        }
        switch(route_controller){
            case CONTROLLER_LOGIN:
                login_controller( conn, ri );
                LOGPRINT( 5, "Controlador LOGIN procesado %d", route_controller );
                return;
            case  CONTROLLER_GAME:
                game_controller( conn, ri, s, route_action, route_param );
                LOGPRINT( 5, "Controlador GAME procesado %d", route_controller );
                return;
        }
    } else {
    }
    LOGPRINT( 5, "Ruta incorrecta => %s", ri->uri );
    render_404( conn, ri );
    return;

}



int   init_webservice( int port ){
    struct mg_context *ctx;
    ctx = mg_start();
    char puerto[1024];
    sprintf( puerto, "%d", port );
    mg_set_option(ctx, "ports", puerto);
    mg_set_uri_callback(ctx, "*", &routes_filter, NULL );

    // Verifico los juegos que hay disponibles
    game_type_discover();
}

