AC_INIT([qgames-server], [0.0.3], [silvioq@gmail.com])
AC_DEFINE([QGS_MAJOR_VERSION],[0], Major version)
AC_DEFINE([QGS_MINOR_VERSION],[0], Minor version)
AC_DEFINE([QGS_REV_VERSION],  [3], Revision version)

AM_INIT_AUTOMAKE
AC_PROG_CC
AC_PROG_RANLIB
AC_PROG_LEX

AC_CONFIG_SRCDIR([configure.ac])

# AC_CHECK_LIB(readline,readline)
# AC_CHECK_LIB(ncurses,initscr,[CURSES_LIBS="-lncurses"])
# AC_CHECK_LIB(ncursesw,initscr,[CURSES_LIBS="-lncursesw"])

AC_CHECK_HEADERS([gd.h])
AC_CHECK_LIB(gd,gdImageCreate)
AC_CHECK_HEADERS([db.h])
AC_CHECK_LIB(db,db_create,, AC_MSG_ERROR([db berkeley no instalada]))

AC_ARG_WITH([qgames],
    [AC_HELP_STRING([--with-qgames=dir],[Establecer directorio del paquete qgames])],
    [],[with_qgames=yes])

AS_IF([test x$with_qgames == xno], AC_MSG_ERROR([No ceo que sea conveniente deshabilitar qgames]))

qgameslibs=
qgamesinc=

# http://gcc.gnu.org/wiki/Regenerating_GCC_Configuration
AS_IF([test x$with_qgames != xyes],
    [
qgameslibs="-L$with_qgames/lib/.libs"
# QGAMESLIBS=${with_qgames}/lib/.libs
qgamesinc="-I$with_qgames/include"
CFLAGS="$CFLAGS $qgamesinc $qgameslibs"
AC_SUBST(qgameslibs) 
# AC_SUBST(QGAMESLIBS) 
AC_SUBST(qgamesinc)
])


AC_CHECK_HEADERS([qgames.h])
AC_CHECK_LIB(qgames,qg_tipojuego_new,[], AC_MSG_ERROR([No esta instalado el core de qgames]))
# CFLAGS="$saved_CFLAGS" 

AC_CHECK_SIZEOF(uint64_t)
AC_CHECK_SIZEOF(time_t)
AS_IF([test $ac_cv_sizeof_uint64_t -lt $ac_cv_sizeof_time_t],
    AC_MSG_ERROR([Tamaño de uint64_t menor a time_t]))

AC_CONFIG_HEADER(config.h)

AC_CONFIG_FILES(
  [Makefile
   webservice/Makefile
   src/Makefile
   test/Makefile
  ])


AC_OUTPUT
