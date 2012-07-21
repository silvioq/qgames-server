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

#ifndef  WEBSERVER_H
#define  WEBSERVER_H



#define  FORMAT_YAML         1
#define  FORMAT_HTML         2
#define  FORMAT_JSON         3
#define  FORMAT_PNG          4
#define  FORMAT_PGN          5
#define  FORMAT_XML          6
#define  FORMAT_QGAME        7

#define  CONTROLLER_LOGIN    1
#define  CONTROLLER_GAME     2
#define  CONTROLLER_HELP     3

#define  ACTION_LOGIN        1
#define  ACTION_LOGOUT       2
#define  ACTION_PING         3
#define  ACTION_WELCOME      4

#define  ACTION_CREA         1
#define  ACTION_TABLERO      2
#define  ACTION_POSIBLES     3
#define  ACTION_MUEVE        4
#define  ACTION_REGISTRA     5
#define  ACTION_DESREGISTRA  6
#define  ACTION_PARTIDA      7
#define  ACTION_TIPOJUEGOS   8
#define  ACTION_IMAGEN       9
#define  ACTION_REGISTRACIONES  10
#define  ACTION_HISTORIAL       11

#define  ACTION_INDEX        1

void login_controller(struct mg_connection *conn, const struct mg_request_info *ri, Session* s, int action, int format );
void game_controller( struct mg_connection* conn, const struct mg_request_info* ri, Session* s, int action, char* parm );
void help_controller( struct mg_connection* conn, const struct mg_request_info* ri, int action, int format );

void render_500(struct mg_connection *conn, const struct mg_request_info *ri, char* buf);
void render_400(struct mg_connection *conn, const struct mg_request_info *ri, char* buf);
void render_403(struct mg_connection *conn, const struct mg_request_info *ri);
void render_404(struct mg_connection *conn, const struct mg_request_info *ri);
void render_200(struct mg_connection *conn, const struct mg_request_info *ri, char* buf);
void render_200f(struct mg_connection *conn, const struct mg_request_info *ri, FILE* f);

extern  int    route_controller;
extern  int    route_action;
extern  int    route_format;

extern  char   route_session[32];
extern  char   route_param[100];
int    get_ruta( char* uri );


/*
 * Retorna una variable "encoded". 
 * Primero verifica las variables GET's y luego las POST's
 * */
char*   get_var( struct mg_connection *conn, const struct mg_request_info *ri, char* var );
int     get_post_data( struct mg_connection *conn, const struct mg_request_info *ri, void** data, int* size );

#endif
