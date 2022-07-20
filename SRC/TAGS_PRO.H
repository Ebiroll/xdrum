/*
 *	tags_protos.h	tags.c and taglist.c prototypes
 *
 *	Copyright © 1996 Jarno Seppänen; see 3MU.c for licensing info.
 */

#ifndef TAGS_PROTOS_H
#define TAGS_PROTOS_H 1

#include "3MU.h"
#include "r_ffp/types.h"

/* taglist.c */
extern struct Tag Tags[];

extern int DefaultTags( const char *srcfilename );
extern void PrintTags( BOOL defaults, BOOL header );
extern int PrintTag( char *tag );

extern struct Tag *ParseTagWithin( char *beg, char *end );
extern struct Tag *ParseTag( const char *name );
extern int SetTagName( const char *name, const char *value );
extern int SetTagNum( unsigned short num, const char *value );
extern int SetTag( struct Tag *tag, const char *value );

#endif
