/*
 *	args_protos.h	args.c prototypes
 *
 *	Copyright © 1996 Jarno Seppänen; see 3MU.c for licensing info.
 */

#ifndef ARGS_PROTOS_H
#define ARGS_PROTOS_H 1

#include "r_ffp/types.h"

extern void ParseEarlyArgs( int argc, char *argv[] );
extern BOOL ParseArgs( int argc, char *argv[], char **src );

#endif
