/*
 *	spec_protos.h	spec.c prototypes
 *
 *	Copyright © 1996 Jarno Seppänen; see 3MU.c for licensing info.
 */

#ifndef SPEC_PROTOS_H
#define SPEC_PROTOS_H 1


#ifdef AMIGA
extern void				*MathBase, *MathTransBase;

extern void OpenLibs( void );
extern void CloseLibs( void );
#endif

#endif
