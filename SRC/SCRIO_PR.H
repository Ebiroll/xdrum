/*
 *	scrio_protos.h	scrio.c prototypes
 *
 *	Copyright © 1996 Jarno Seppänen; see 3MU.c for licensing info.
 */

#ifndef SCRIO_PROTOS_H
#define SCRIO_PROTOS_H 1

#include "r_ffp/r_ffp.h"
#include "r_ffp/types.h"

extern BOOL				Verbose;
extern unsigned long	UpdateSize;

extern void InitPercent( unsigned long size );
extern void UpdPercent( int inc, r_float sample );
extern void FinalPercent( void );
extern int  ScrWidth( void );

#endif
