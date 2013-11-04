
check_PROGRAMS = test-main  test-db-create-file  \
				test-dup-user test-create-user test-create-game-type test-cursor-game-type \
				test-create-session test-create-game test-check-user-game \
				test-del-game  \
				test-routes

QGAMESLIBS=@QGAMESLIBS@
LIB=$(QGAMESLIBS)
DEPENDENCES= ../src/dbmanager.c  ../src/dbmanager.h \
		../src/users.h  ../src/users.c \
		../src/games.h  ../src/games.c \
		../src/session.h  ../src/session.c \
		../common/log.h  ../common/log.c \
		../common/packer.h  ../common/packer.c \
		../common/md5.h  ../common/md5.c 

test_main_SOURCES = test-main.c

test_db_create_file_SOURCES = $(DEPENDENCES) test-db-create-file.c
test_dup_user_SOURCES       = $(DEPENDENCES) test-dup-user.c
test_create_user_SOURCES    = $(DEPENDENCES) test-create-user.c
test_check_user_game_SOURCES  = $(DEPENDENCES) test-check-user-game.c
test_create_game_type_SOURCES = $(DEPENDENCES) test-create-game-type.c
test_create_game_SOURCES    = $(DEPENDENCES) test-create-game.c
test_del_game_SOURCES       = $(DEPENDENCES) test-del-game.c
test_create_session_SOURCES = $(DEPENDENCES) test-create-session.c
test_cursor_game_type_SOURCES = $(DEPENDENCES) test-cursor-game-type.c

test_routes_SOURCES         = ../webservice/webserver.h \
		test-routes.c   


test_routes_LDADD =  $(top_srcdir)/webservice/libwebserver.a

AM_CFLAGS = -I ../src -I ../common -I ../webservice

TESTS = $(check_PROGRAMS) webservice/test-*.sh
EXTRA_DIST = $(top_srcdir)/test/test.qgame
