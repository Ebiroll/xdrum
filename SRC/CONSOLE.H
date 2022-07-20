/*
 *	console.h		console controlling
 *
 *	Copyright © 1996 Jarno Seppänen; see 3MU.c for licensing info.
 */

#ifndef CONSOLE_H
#define CONSOLE_H

/* näytön oletusarvoinen leveys, minimi- ja maksimiarvot */
#define WIDTH 75
#define MINWIDTH 20
#define MAXWIDTH 500

#ifdef AMIGA
#define ANSI_TERMINAL
#endif
#ifdef unix
#define ANSI_TERMINAL
#endif
#ifdef MSDOS
#define PC_TERMINAL
#endif

#ifdef ANSI_TERMINAL

/* CSI <type> ; <30+fgcolor> ; <40+bgcolor> m */

#define CSI "›"

/* underlining */
#if 0
#define	ULON	CSI "4;31;40m"
#define	ULOFF	CSI "0;31;40m"
#else
#define	ULON	CSI "4;;m"
#define	ULOFF	CSI "0;;m"
#endif

/* italic */
#if 0
#define ION		CSI "3;31;40m"
#define	IOFF	CSI "0;31;40m"
#else
#define ION		CSI "3;;m"
#define	IOFF	CSI "0;;m"
#endif

/* vertical motion */
#define fwd(a)	CSI a "C"
#define bwd(a)	CSI a "D"

#else
#ifdef PC_TERMINAL

#define CSI     "["
#define	ULON	CSI "4;40m"
#define	ULOFF	CSI "0;40m"
#define ION		CSI "3;40m"
#define	IOFF	CSI "0;40m"
#define fwd(a)	CSI a "C"
#define bwd(a)	CSI a "D"

#else

#define CSI     ""
#define ULON    ""
#define ULOFF   ""
#define ION     ""
#define IOFF    ""
#define fwd(a)  ""
#define bwd(a)  ""

#endif
#endif
#endif
