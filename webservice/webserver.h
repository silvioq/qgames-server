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


void login_controller(struct mg_connection *conn, const struct mg_request_info *ri);
void login_view_xml( struct mg_connection *conn, const struct mg_request_info* ri, Session* s );

#define  ACTION_CREA      1
#define  ACTION_TABLERO   2
#define  ACTION_POSIBLES  3
#define  ACTION_MUEVE     4
#define  ACTION_REGISTRA  5
#define  ACTION_DESREGISTRA  6

void game_controller( struct mg_connection* conn, const struct mg_request_info* ri, Session* s, int action, char* parm );



void list_controller( struct mg_connection *conn, const struct mg_request_info* ri, Session* s );
void list_view_xml(  struct mg_connection *conn, const struct mg_request_info* ri, Session* s );

void render_500(struct mg_connection *conn, const struct mg_request_info *ri, char* buf);
void render_400(struct mg_connection *conn, const struct mg_request_info *ri, char* buf);
void render_403(struct mg_connection *conn, const struct mg_request_info *ri);
void render_404(struct mg_connection *conn, const struct mg_request_info *ri);
void render_200(struct mg_connection *conn, const struct mg_request_info *ri, char* buf);
void render_200f(struct mg_connection *conn, const struct mg_request_info *ri, FILE* f);


#endif
