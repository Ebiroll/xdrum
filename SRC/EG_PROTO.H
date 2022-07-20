/*
 *	eg_protos.h		eg.c prototypes
 *
 *	Copyright © 1996 Jarno Seppänen; see 3MU.c for licensing info.
 */

#ifndef EG_PROTOS_H
#define EG_PROTOS_H 1

#include "r_ffp/r_ffp.h"
#include "r_ffp/types.h"

extern r_float EG( r_float a, r_float d, r_float s, r_float r,
			char acc, char len, BOOL trig, char nextmode, r_float time );

#endif
