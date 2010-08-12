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
#include  <time.h>
#include  "mongoose.h"
#include  "users.h"
#include  "log.h"
#include  "session.h"

static void render_400(struct mg_connection *conn, const struct mg_request_info *ri, char* buf);
static void render_404(struct mg_connection *conn, const struct mg_request_info *ri);
static void render_200(struct mg_connection *conn, const struct mg_request_info *ri, char* buf);

static void login_controller(struct mg_connection *conn, const struct mg_request_info *ri){
    char* user = mg_get_var( conn, "user" );
    char* pass = mg_get_var( conn, "pass" );
    if( strcmp( ri->request_method, "POST" ) != 0 ){
        render_400( conn, ri, "Debe enviar login por POST" );
        return;
    }
    if( user && pass ){
        User* u = user_find_by_code( user );
        if( !u ){
            render_400( conn, ri, "Usuario o password incorrecta" );
        } else {
            if( user_check_password( u, pass ) ){
                render_200( conn, ri, "Usuario perfecto" );
            } else {
                render_400( conn, ri, "Usuario o password incorrecta" );
            }
            user_free( u );
        }
    } else {
        render_400( conn, ri, "Debe enviar usuario y password" );
    }
    if( user ) mg_free( user );
    if( pass ) mg_free( pass );
    return;
}

static void render_404(struct mg_connection *conn, const struct mg_request_info *ri){
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

static void render_400(struct mg_connection *conn, const struct mg_request_info *ri, char* buf){
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

static void render_200(struct mg_connection *conn, const struct mg_request_info *ri, char* buf){
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

static void routes_filter(struct mg_connection *conn, const struct mg_request_info *ri, void *data){
    char x[33];
    char buff[1024];
    int ret;

    // Ruta login
    ret = sscanf( "/login%s", buff );
    if( ret == -1 ){
        LOGPRINT( 5, "Ruteando a controlador login => %s", ri->uri );
        login_controller( conn, ri );
        return;
    } else if( ret > 0 ){
        render_404( conn, ri );
        return;
    }

    
    
    if( sscanf( "/%32s/", ri->uri, &x ) ){
    }
    
}



int   init_webservice( int port ){
    struct mg_context *ctx;
    ctx = mg_start();
    char puerto[1024];
    sprintf( puerto, "%d", port );
    mg_set_option(ctx, "ports", puerto);
    mg_set_uri_callback(ctx, "*", &routes_filter, NULL );
}

