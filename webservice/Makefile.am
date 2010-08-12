
bin_PROGRAMS = qgserverd

qgserverd_SOURCES = qgserverd.c webserver.c \
		session.c  session.h \
		mongoose.c  mongoose.h \
		../src/dbmanager.c  ../src/dbmanager.h \
		../src/users.h  ../src/users.c \
		../src/games.h	 ../src/games.c \
		../common/log.h  ../common/log.c \
		../common/packer.h  ../common/packer.c \
		../common/md5.h  ../common/md5.c 

AM_CFLAGS=-I $(top_srcdir)/common -I $(top_srcdir)/src -DNO_SSL -DNO_CGI
