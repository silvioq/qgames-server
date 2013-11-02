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
#include  <stdlib.h>
#include  <sys/time.h>
#include  <time.h>
#include  <config.h>
#include  "mongoose.h"
#include  "users.h"
#include  "log.h"
#include  "session.h"
#include  "cJSON.h"
#include  "webserver.h"

/*
 * Este es el controlador de login.
 * Lo que voy a hacer es sencillo. 
 * Primero, controlo que la password sea correcta.
 * Segundo, devuelvo una nueva sesion
 * */

static void login(struct mg_connection *conn, const struct mg_request_info *ri, int format){
    if( strcmp( ri->request_method, "POST" ) != 0 ){
        render_400( conn, ri, "Debe enviar login por POST" );
        return;
    }
    char* user = get_var( conn, ri, "user" );
    char* pass = get_var( conn, ri, "pass" );

    if( user && pass ){
        User* u = user_find_by_code( user );
        if( !u ){
            render_400( conn, ri, "Usuario o password incorrecta" );
        } else {
            if( user_check_password( u, pass ) ){
                Session* s = session_new( u );
                if( session_save( s ) ){
                    cJSON* r = cJSON_CreateObject();
                    char sid[33]; memcpy( sid, s->id, 32 ); sid[32] = 0;
                    cJSON_AddStringToObject( r, "respuesta", "OK" );
                    cJSON_AddStringToObject( r, "sesion", sid );
                    cJSON_AddStringToObject( r, "version", PACKAGE_VERSION);
                    render_200j( conn, ri, r, format );
                    cJSON_Delete( r );
                } else {
                    render_500( conn, ri, "Error al grabar sesion en " __FILE__ ":" QUOTEME(__LINE__) );
                }
                session_free( s );
            } else {
                render_400( conn, ri, "Usuario o password incorrecta" );
            }
            user_free( u );
        }
    } else {
        render_400( conn, ri, "Debe enviar usuario y password" );
    }
    if( user ) free( user );
    if( pass ) free( pass );
    dbact_sync();
    return;
}

static void logout(struct mg_connection *conn, const struct mg_request_info *ri, Session* s, int format ){
    session_close( s );
    cJSON* j = cJSON_CreateObject( );
    cJSON_AddStringToObject( j, "respuesta", "OK" );
    cJSON_AddStringToObject( j, "texto", "Session cerrada" );
    render_200j( conn, ri, j, format );
    cJSON_Delete( j );
}

void login_controller(struct mg_connection *conn, const struct mg_request_info *ri, Session *s, int action, int format){
    switch( action ){
        case ACTION_LOGIN:
            login( conn, ri, format );
            break;
        case ACTION_LOGOUT:
            logout( conn, ri, s, format ? format : FORMAT_TXT );
            break;
        case ACTION_PING:
            render_200( conn, ri, "pong" );
            break;
        case ACTION_WELCOME:
            render_200( conn, ri, "Welcome to qgserverd " VERSION "\nUse: /help para ver comandos\n" );
            break;
        default:
            render_404( conn, ri );
    }
}
