/*
 *	vcf_protos.h	vcf.c prototypes
 *
 *	Copyright © 1996 Jarno Seppänen; see 3MU.c for licensing info.
 */

#ifndef VCF_PROTOS_H
#define VCF_PROTOS_H 1

#include "r_ffp/r_ffp.h"

extern long VCFinit( char VCFpoles, char VCFtype );
extern long VCFquit( void );
extern r_float VCF( r_float inp, r_float env, r_float cutoff, r_float reso );

#endif
