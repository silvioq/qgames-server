bin_PROGRAMS = qgserver-tool qgserverd


qgserver_tool_SOURCES = qgserver-tool.c \
		dbmanager.c  dbmanager.h \
		users.h  users.c \
		games.h	 games.c \
		../common/log.h  ../common/log.c \
		../common/packer.h  ../common/packer.c \
		../common/md5.h  ../common/md5.c

AM_CFLAGS=-I $(top_srcdir)/common -I$(top_srcdir)/webservice -DNO_SSL -DNO_CGI

qgserverd_SOURCES = qgserverd.c \
		session.c     session.h \
		dbmanager.c   dbmanager.h \
		users.h  			users.c \
		games.h	      games.c \
		../common/log.h  ../common/log.c \
		../common/packer.h  ../common/packer.c \
		../common/md5.h  ../common/md5.c \
		../common/base64.h ../common/base64.c \
    ../common/cJSON/cJSON.c  ../common/cJSON/cJSON.h

qgserverd_LDADD = $(top_srcdir)/webservice/libwebserver.a
