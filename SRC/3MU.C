/*
 *	3MU.c			Roland TB-303 Bassline 3mulator v1.1
 *
 *	Copyright ©†1995, 1996 by Jarno Sepp‰nen <jarno.seppanen@cc.tut.fi>
 *	Files eg.c, seq.c, vcf.c and vco.c are based on a routine
 *	by Lars Hamre <lars@scala.no>. The file orig-tb.c contains the whole
 *	original routine as posted to USENET by Lars. File LICENSE contains
 *	the GNU General Public License.
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version 2
 *	of the License, or (at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#define BETANUM "25"	/* juokseva numero	*/
#define MAIN 1			/* debug.h:lle		*/

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "3MU.h"
#include "banner.h"
#include "console.h"
#include "debug.h"

#include "r_ffp/r_ffp.h"
#include "args_pro.h"
#include "io_proto.h"
#include "parse_pr.h"
#include "prep_pro.h"
#include "misc_pro.h"
#include "scrio_pr.h"
#include "spec_pro.h"
#include "tags_pro.h"
#include "seq_prot.h"

/* maksimi rivinpituus */
#define MAXLINELEN	200

	/* scrio.c	*/
extern void PrintInfo( struct Settings *d );
extern void Analyse( clock_t clock_diff, long length );
	/* misc.c	*/
extern void Check( struct Settings *set );

		/* ---	   Funktioiden prototyypit	--- */


static void Args( int argc, char *argv[] );
static void Calc( void );
static void Source( void );
void Pois( void );
void IntHandler( int s );
void FPEhandler( int s );

		/* ---		  Muuttujat		--- */

static char *srcname = NULL;

#ifdef DEBUG
int DebugLevel = 0;
#endif

		/* ---		   Funktiot		--- */

/*		main()
 *
 *	3MU:n p‰‰rutiini; kaiken alku
 */

#ifdef OLLES_TEST
void main( int argc, char *argv[] );


void main( int argc, char *argv[] )
{
	/* asennetaan lopetuskoodi ja signaalien k‰sittelij‰ */
	atexit( Pois );
	signal( SIGINT, IntHandler );	/* ^C */
	/*	signal( SIGFPE, FPEhandler ); */
	signal( SIGFPE, SIG_IGN );	/* ylivuoto/alivuoto tms. */

	Args( argc, argv );

#ifdef AMIGA
#ifdef DEBUG
	DEBUGMSG( 2, "main: open libraries" )
#endif
	OpenLibs();
#endif

#ifdef DEBUG
	DEBUGMSG( 2, "main: set default values" )
#endif
	if( DefaultTags( srcname ))
		exit( 5 );

	Source();

#ifdef DEBUG
	DEBUGMSG( 2, "main: check versions" )
#endif
	Check( &Data );

#ifdef DEBUG
	DEBUGMSG( 2, "main: parse delayed options" )
#endif
	if( ParseArgs( argc, argv, NULL ))
		exit( 0 );

#ifdef DEBUG
	IFDEBUG( 1 )
		DumpAllData( &Data, Sequence );
#endif

	if( OpenFiles())
		exit( 20 );

#ifdef DEBUG
	DEBUGMSG( 2, "main: prepare data" )
#endif
	if( Prep( &Data, Sequence ))
		exit( 20 );

#ifdef DEBUG
	IFDEBUG( 1 )
		DumpAllData( &Data, Sequence );
#endif

	PrintInfo( &Data );
	Calc();
		
#ifdef DEBUG
	DEBUGMSG( 2, "main: exit" )
#endif
	exit( 0 );
}

#endif


/*		void Args( int argc, char *argv[] );
 *
 *	Kaikkea komentoriviargumentteihin liittyv‰‰.
 */

static void Args( int argc, char *argv[] )
{
#ifdef AMIGA
	/* no Amiga Workbench support: */
	if( !argc )
		exit( 20 );
#endif

#ifdef DEBUG
	DEBUGMSG( 2, "main: parse early arguments" )
#endif
	ParseEarlyArgs( argc, argv );

	if( Verbose )
		puts( BANNER );

	if( argc == 1 )
	{
		puts( BIGBANNER );
		getchar();
		puts( USAGE );
		exit( 5 );
	}

#ifdef DEBUG
	DEBUGMSG( 2, "main: parse arguments" )
#endif
	if( ParseArgs( argc, argv, &srcname ))
		exit( 0 );
}

/*		void Calc( void );
 *
 *	Laskee.
 */

static void Calc( void )
{
	long tberr, len;
	clock_t calctime;

#ifdef DEBUG
	DEBUGMSG( 2, "main: calculate data" )
#endif

	/* alusta laskuri */
	InitPercent( TBsize( &Data, Sequence ));

	len = TBlen( &Data, Sequence );

	ResetTimer();
	StartTimer();
	tberr = TB( &Data, Sequence );
	StopTimer();
	calctime = ReadTimer();

	if( tberr )
		exit( 20 );

	/* tulosta lopullinen laskurin arvo */
	FinalPercent();

	if( Verbose )
		Analyse( calctime, len );
}

/*		void Source( void );
 *
 *	Lukee ja tulkkaa l‰hdekoodin.
 */

static void Source( void )
{
	FILE *input;
	char buf[ MAXLINELEN ];
	long err = 0;

	if( srcname != NULL )
	{
#ifdef DEBUG
		DEBUGMSG( 2, "main: open source file" )
#endif
		input = OpenSource( srcname );
		if( input == NULL )
			exit( 20 );
	}
	else
	{
#ifdef DEBUG
		DEBUGMSG( 2, "main: sourcing STDIN" )
#endif
		input = stdin;
	}

#ifdef DEBUG
	DEBUGMSG( 2, "main: parse source" )
#endif

	if( fgets( buf, sizeof( buf ), input ))
		err = StartParse( buf, &Data, Sequence );

	while( !err && fgets( buf, sizeof( buf ), input ))
		err = Parse( buf );

	if( StopParse())
		err = 1;

	if( srcname != NULL )
	{
#ifdef DEBUG
		DEBUGMSG( 2, "main: close source file" )
#endif
		fclose( input );
	}
	if( err )
		exit( 20 );
}

/*		void Pois( void );
 *
 *	Kaiken loppu: lopettaa 3MU:n suorituksen ja palauttaa k‰yttˆj‰rjestelm‰lle
 *	paluuarvon retval. Sulkee kirjastot, vapauttaa l‰hdekoodin puskurin jne.
 */

void Pois( void )
{
	/* estet‰‰n poistumisen keskeytt‰minen: */
	signal( SIGINT, SIG_IGN );	/* ^C */

	/* sulje tiedostot */
	CloseFiles();

#ifdef DEBUG
	DEBUGMSG( 2, "main: free textvars" )
#endif
 	SetTextVar( &Data.IDstring, NULL );
 	SetTextVar( &Data.WaveFilename, NULL );
 	SetTextVar( &Data.OutFilename, NULL );

#ifdef AMIGA
#ifdef DEBUG
	DEBUGMSG( 2, "main: close libs" )
#endif
	CloseLibs();
#endif
}

/*		void IntHandler( int s );
 *
 *	K‰sittelee int-signaalin. Jos tulee SIGINT eli ^C, lopetetaan ohjelma.
 */

void IntHandler( int s )
{
	if( s != SIGINT )
		return;

	fputs( "\nExiting...\n", stderr );
	exit( 1 );
}

/*		void FPEhandler( int s );
 *
 *	K‰sittelee liukulukusignaalin. Jos tulee SIGFPE, kerrotaan jotakin k‰ytt‰j‰lle ja
 *  lopetetaan ohjelma.
 */

void FPEhandler( int s )
{
	if( s != SIGFPE )
		return;

	fputs( "\nA floating point exception has occurred. An overflow is the probable cause\n"
		   "and since the output would contain nothing but noise, we'll stop here.\n"
		   "Try adjusting the source values.\n", stderr );
	if( Data.VCFtype == FILT_HAMRE )
		fputs( "The `hamre' filter can get quite chaotic sometimes, so changing\n"
			   "the value of filter-type to 'generic' should suffice.\n", stderr );
	exit( 1 );
}

		/* ---		    Loppu		--- */
