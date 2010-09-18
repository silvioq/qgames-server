noinst_LIBRARIES = libwebserver.a

libwebserver_a_SOURCES = \
		webserver.c   webserver.h \
    app/login.c  \
    app/game.c  \
		mongoose.c    mongoose.h \
		routes.l   

libwebserver_a_LIBADD = $(LIBOBJS)
AM_CFLAGS=-I $(top_srcdir)/common -I$(top_srcdir)/src -DNO_SSL -DNO_CGI

