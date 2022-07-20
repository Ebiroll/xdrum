/*
 *	misc_protos.h	misc.c prototypes
 *
 *	Copyright © 1996 Jarno Seppänen; see 3MU.c for licensing info.
 */

#ifndef MISC_PROTOS_H
#define MISC_PROTOS_H 1

#include <stdio.h>
#include <time.h>
#include "3MU.h"

#include "r_ffp/r_ffp.h"
#include "r_ffp/types.h"

extern char *EnvironName, *EnvironVersion;
extern char *ReqEnvName, *ReqEngName, *ReqEnvVers, *ReqEngVers;
extern struct Settings	Data;
extern struct Note		Sequence[];
extern FILE				*infile, *outfile;

extern BOOL CmpSbeg( const char *first, const char *second );
extern BOOL CmpS( const char *first, const char *second );
extern int ReadNum( const char *text, int *dest, int min, int max );

extern r_float AvgLevel( r_float sample, r_float srate );

extern void StartTimer( void );
extern void StopTimer( void );
extern clock_t ReadTimer( void );
extern void ResetTimer( void );

extern void Comma2Period( char *a );

#endif
