/*
 *	io_protos.h		io.c prototypes
 *
 *	Copyright © 1996 Jarno Seppänen; see 3MU.c for licensing info.
 */

#ifndef IO_PROTOS_H
#define IO_PROTOS_H 1

#include <stdio.h>

#include "r_ffp/r_ffp.h"
#include "r_ffp/types.h"

extern long OpenFiles( void );
extern long CloseFiles( void );
extern long ReadSample( FILE *file, r_float *fin, unsigned char format );
extern long WriteSample( FILE *file, r_float fout, unsigned char format );
extern FILE *OpenSource( const char *srcname );

extern BOOL		Query;
extern size_t	IObufSize;

#endif
