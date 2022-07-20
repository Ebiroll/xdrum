/*
 *	debug.h			debugging
 *
 *	Copyright © 1996 Jarno Sepp‰nen; see 3MU.c for licensing info.
 */

#ifndef DEBUG_H
#define DEBUG_H 1

/* seuraavan rivin voi kommentoida pois, jolloin debuggaus h‰vi‰‰ */
/*#define DEBUG 1*/

#ifdef DEBUG

#include <stdio.h>
#include "3MU.h"

#define IFDEBUG(l)		if(DebugLevel>=(l))
#define DEBUGMSG(l,s)	{if(DebugLevel>=(l)){fputc('+',stderr);fputs(s,stderr);fputc('\n',stderr);}}

#ifndef MAIN
extern int DebugLevel;
#endif

/* protos: */

extern void DumpAllData( struct Settings *a, struct Note *s );
extern void DumpSettings( struct Settings *a );
extern void DumpNote( struct Note *n );

#endif	/* DEBUG	*/

#endif	/* DEBUG_H	*/
