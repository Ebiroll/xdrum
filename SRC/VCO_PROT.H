/*
 *	vco_protos.h	vco.c prototypes
 *
 *	Copyright © 1996 Jarno Seppänen; see 3MU.c for licensing info.
 */

#ifndef VCO_PROTOS_H
#define VCO_PROTOS_H 1

#include "r_ffp/r_ffp.h"
#include "r_ffp/types.h"

extern long VCOinit( char oscmode, char hpfilt,
				unsigned short tlen, r_float *taddr, r_float tcycles,
				r_float pratio, r_float initrfreq, r_float portalength,
				r_float noportalength );
extern r_float VCO( r_float rfreq, r_float env, BOOL porta, BOOL trig );

#endif
