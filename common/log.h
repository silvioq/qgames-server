/*
  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.
  Silvio Quadri 2009.
*/

#ifndef  LOG_H
#define  LOG_H

#include  <stdio.h>

/*
 * loglevel puede ser
 * 1. Error severo
 * 2. Error
 * 3. Warning
 * 4. Noticia
 * 5. Informacion
 * */

extern   FILE*  logfile;
extern   int    loglevel;

#ifndef QUOTEME
#define QUOTEME_(x) #x
#define QUOTEME(x) QUOTEME_(x)
#endif

#define  LOGPRINT( level, fmt, ... )  \
    if( level <= loglevel ){ \
        if( !logfile ) logfile = stdout; \
        fprintf( logfile, "Lev %02d: %-20s " fmt "\n", level, __FILE__ ":" QUOTEME(__LINE__), ##__VA_ARGS__ ); \
    }

#endif
