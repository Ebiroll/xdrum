/*
 *	seq_protos.h		tb.c prototypes
 *
 *	Copyright © 1996 Jarno Seppänen; see 3MU.c for licensing info.
 */

#ifndef SEQ_PROTOS_H
#define SEQ_PROTOS_H 1

#include "3MU.h"

#include "r_ffp/r_ffp.h"
#include "r_ffp/types.h"

extern char *EngineName, *EngineVersion;

extern long TB( struct Settings *dat, struct Note *not );
extern unsigned long TBlen( struct Settings *dat, struct Note *not );
extern unsigned long TBsize( struct Settings *dat, struct Note *not );
extern r_float TBdur( struct Settings *dat, struct Note *not );
extern unsigned short TBnoteDur( struct Settings *dat, struct Note *not,
						long inlen );
extern r_float PeakLevel( void );
extern r_float OptimalGain( r_float Gain );

#endif
