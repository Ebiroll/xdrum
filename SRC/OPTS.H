/*
 *	opts.h
 *
 *	Copyright © 1996 Jarno Seppänen; see 3MU.c for licensing info.
 */

#ifndef OPTS_H
#define OPTS_H

#include "debug.h"

#define	OPT_IOBUF	0
#define	OPT_INFREQ	1
#define	OPT_INFILE	2
#define	OPT_INFORM	3
#define	OPT_PIJUMP	4
#define	OPT_OUTFREQ	5
#define	OPT_OUTFILE	6
#define	OPT_OUTFORM	7
#define OPT_QUIET	8
#define	OPT_STDIN	9
#define	OPT_TAG		10
#define	OPT_VERSION	11
#define OPT_OVERWR	12

#ifndef DEBUG
#define	NUM_OPTS	13
#else
#define OPT_DEBUG	13
#define	NUM_OPTS	14
#endif

	/* aikaisin tutkittavat optiot */
/* hiljaisuuden liipaisu */
#define QUIETOPT	"-q"
#define	STDOUTOPT	"-o"
/* debuggauksen liipaisu */
#define DEBUGOPT	"-d"

#endif
