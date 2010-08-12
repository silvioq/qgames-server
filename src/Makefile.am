bin_PROGRAMS = qgserver-tool qgserverd


qgserver_tool_SOURCES = qgserver-tool.c \
		dbmanager.c  dbmanager.h \
		users.h  users.c \
		games.h	 games.c \
		../common/log.h  ../common/log.c \
		../common/packer.h  ../common/packer.c \
		../common/md5.h  ../common/md5.c 

AM_CFLAGS=-I $(top_srcdir)/common -DNO_SSL -DNO_CGI

qgserverd_SOURCES = qgserverd.c \
		$(top_srcdir)/webservice/webserver.c \
		$(top_srcdir)/webservice/mongoose.c    $(top_srcdir)/webservice/mongoose.h \
		session.c     session.h \
		dbmanager.c   dbmanager.h \
		users.h  			users.c \
		games.h	      games.c \
		../common/log.h  ../common/log.c \
		../common/packer.h  ../common/packer.c \
		../common/md5.h  ../common/md5.c 

