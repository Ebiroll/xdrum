/*
 *	banner.h		banner and usage
 *
 *	Copyright © 1996 Jarno Seppänen; see 3MU.c for licensing info.
 */

#ifndef BANNER_H
#define BANNER_H 1

#include "3MU.h"
#include "console.h"
#include "debug.h"


#define JAMS_EMAIL   "jams@cs.tut.fi"
#define JAMS_URL     "http://www.modeemi.cs.tut.fi/~jams/"
#ifndef MS_DOS
#define JAMS_NIMI    "Jarno Seppänen"
#else
#define JAMS_NIMI    "Jarno Sepp„nen"
#endif

#ifdef DEBUG
/* #define DEBUGTXT	"DE" ION "BUG " IOFF */
#define DEBUGTXT	"DEBUG "
#define DUSAGE "\n -d [level]\t\tPrint debugging information (set debug level)"
#else
#define DEBUGTXT	""
#define DUSAGE		""
#endif

/*
#define BANNER \
"   __       _  _ _\n" \
"  (_ )/) _  _)/ )_)  3MU the Roland TB-303 Bassline 3mulator\n" \
"   ///_)   _)(_/_)   " DEBUGTXT "Version " ENVVERS " beta #" BETANUM " (" __DATE__ ")\n"
*/

#define BANNER \
"3MU, a TB-303 Bassline 3mulator---" DEBUGTXT "version " ENVVERS " beta #" BETANUM " (" __DATE__ ")\n"

#define BIGBANNER \
"\tCopyright © 1995, 1996 " JAMS_NIMI ".\n" \
"\tWritten by " JAMS_NIMI " <" JAMS_EMAIL ">;\n" \
"\tbased on a routine by Lars Hamre <lars@scala.no>.\n" \
"\tReleased in 1996 under GNU General Public License:\n" \
"\t - 3MU comes with ABSOLUTELY NO WARRANTY.\n" \
"\t - This is free software, and you are welcome to\n" \
"\t   redistribute it under certain conditions.\n" \
"\t - Read LICENSE for details.\n" \
"\tDocumentation @ <URL:" JAMS_URL "3MU/>\n" \
"\n" \
"\t\t\tPress RETURN to see usage."

#define USAGE \
"Usage: 3MU [source filename] [options]\n"\
"Options:\n"\
" -b <size>\t\tSet I/O buffer size\n"\
" -i [filename]\t\tForce input waveform filename (STDIN if no name specified)\n"\
" -if <frequency>\tForce input waveform frequency\n"\
" -it <type>\t\tForce input waveform type (8-bit, 16-bit or float)\n"\
" -j <jump>\t\tSet progress indicator jump (0=no indication)\n"\
" -o [filename]\t\tForce output file name (STDOUT if no name specified)\n"\
" -of <frequency>\tForce output frequency\n"\
" -ot <type>\t\tForce type of output data (8-bit, 16-bit or float)\n"\
" -O\t\t\tOverwrite without asking\n"\
" -q\t\t\tBe quiet\n"\
" -s\t\t\tSource STDIN (no source filename needed)\n"\
" -t\t\t\tPrint tag names\n"\
" -t <tag>\t\tPrint tag value\n"\
" -t <tag>=<value>\tSet tag to value\n"\
" -v\t\t\tPrint version information" DUSAGE

#endif	/* BANNER_H */
