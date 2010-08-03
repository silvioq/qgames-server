check_PROGRAMS = test-main  test-db-create-file  test-dup-user test-create-user

QGAMESLIBS=@QGAMESLIBS@
LIB=$(QGAMESLIBS)
DEPENDENCES= ../src/dbmanager.c  ../src/dbmanager.h \
		../src/users.h  ../src/users.c \
		../common/log.h  ../common/log.c \
		../common/md5.h  ../common/md5.c 

test_main_SOURCES = test-main.c

test_db_create_file_SOURCES = $(DEPENDENCES) test-db-create-file.c
test_dup_user_SOURCES       = $(DEPENDENCES) test-dup-user.c
test_create_user_SOURCES    = $(DEPENDENCES) test-create-user.c

AM_CFLAGS = -I ../src -I ../common

TESTS = $(check_PROGRAMS)

