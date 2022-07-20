/*
 *	parse_protos.h	parse.c and parse2.c prototypes
 *
 *	Copyright © 1996 Jarno Seppänen; see 3MU.c for licensing info.
 */

#ifndef PARSE_PROTOS_H
#define PARSE_PROTOS_H 1

#include "3MU.h"

#include "r_ffp/types.h"

/* parse.c */
extern long StartParse( char *buffer, struct Settings *Data, struct Note *Sequ );
extern long Parse( char *buffer );
extern long StopParse( void );


/* parse2.c */
extern char *SetTextVar( char **var, char *text );
extern char *InterpTagType( struct Tag *tag, const char *beg, void *var );

#endif
