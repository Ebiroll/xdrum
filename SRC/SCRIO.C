/*
 *	scrio.c			screen i/o
 *
 *	Copyright © 1996 Jarno Seppänen; see 3MU.c for licensing info.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "3MU.h"
#include "console.h"
#include "convaud.h"
#include "debug.h"

#include "misc_pro.h"
#include "r_ffp/r_ffp_pr.h"
#include "seq_prot.h"

#define UPDSTRING \
	"   0%% 00:00 (       0/%8ld) [----------+---------+---------+---------]\r"
#define UPDSTRINGCLR \
	"                                                                           \r"
#define UPDSTRINGLEN	72
#define UPDBARLEN		40
#define	UPDBARLEVELS	5

		/* ---	   Funktioiden prototyypit	--- */

/* vain main():n käytössä */
void PrintInfo( struct Settings *d );

void InitPercent( unsigned long size );
void UpdPercent( int inc, r_float sample );
void FinalPercent( void );
void Analyse( clock_t clock_diff, long length );
int  ScrWidth( void );

		/* ---		  Muuttujat		--- */

BOOL			Verbose		= 1;
unsigned long UpdateSize	= 100;

/* progress indicator variables: */
static unsigned long psize;
static char Levels[ UPDBARLEVELS + 2 ] = { ' ', '_', '.', 'o', 'O', '@', '*'  };
/*	indicator	signal level
 *		`*'		>100% (clip)
 *		`@'		80...100%
 *		`O'		60...80%
 *		`o'		40...60%
 *		`.'		20...40%
 *		`_'		0...20%
 *		` '		silence
 */
static char Working[ 4 ] = { '-', '\\', '|', '/' };

		/* ---		   Funktiot		--- */

/*		void PrintInfo( struct Settings *d );
 *
 *	Kertoo käyttäjälle mitä on tapahtumassa.
 */

void PrintInfo( struct Settings *d )
{
	if( !Verbose )
		return;
	if( d->OutFilename != NULL )
		printf( "Producing audio file `%s'.\n", d->OutFilename );
	if( d->IDstring != NULL )
		printf( "Bassline ID: %s\n", d->IDstring );
}

/*		void InitPercent( unsigned long size );
 *
 *	Alustaa prosenttilaskurin eli mm. tulostaa 'please wait...'-rivin.
 *	size on tiedoston koko.
 */

void InitPercent( unsigned long size )
{
#ifdef DEBUG
	IFDEBUG( 20 )	/* sTRICTLY DEBUG */
		return;
#endif
	if( !Verbose )
		return;
	if( fputs( "Processing bassline...Press CTRL-C to abort\n", stdout ) == EOF )
		return;
	if( fflush( stdout ) == EOF )
		return;
#ifndef ANSI_TERMINAL
#ifndef PC_TERMINAL
	UpdateSize=0;
#endif
#endif
	if( !UpdateSize )
		return;
	if( UpdateSize < sizeof( r_float ))
		UpdateSize = sizeof( r_float );
	printf( UPDSTRING, size );
	if( fflush( stdout ) == EOF )
		return;
	psize = size;
}

/*		void UpdPercent( int inc, r_float sample );
 *
 *	Päivittää prosenttilaskurin. inc on tallennetun tiedon pituus tavuina eli
 *	kohdan lisäys. Tulostaa uuden prosentti- ym. arvon, mikäli on aika.
 *	sample on tulosteen näyte [-1, 1]
 */

void UpdPercent( int inc, r_float sample )
{
	static int currwork = 0, currperc = -1, currbarlev = -1, currbarlen = -1;
	static int curreta = 0;
	static clock_t workclock = 0, beginclock = 0;
	static unsigned long bytes = 0;
	int perc, eta, barlen, barlev;
	BOOL updwork = 0, updperc = 0, updeta = 0, updbytes = 0, updbar = 0;
	unsigned long preupdbytes;
	r_float avg;
	clock_t currclock;

#ifdef DEBUG
	IFDEBUG( 20 )	/* sTRICTLY DEBUG */
		return;
#endif
	if( !UpdateSize || !Verbose || !psize )
		return;

	/* alustus */
	if( bytes == 0 )
	{
		updwork = updperc = updeta = updbytes = updbar = 1;
		beginclock = clock();
		workclock = beginclock + CLOCKS_PER_SEC;
	}

	/* lasketaan tavut */
	preupdbytes = bytes % UpdateSize;
	bytes += inc;
	if( preupdbytes >= bytes % UpdateSize )
		updbytes = 1;

	/* lasketaan prosentti */
	perc = ( 100L * bytes ) / psize;

	/* päätellään työskentelyindikaattori */
	currclock = clock();
	if( currclock >= workclock )
	{
		workclock = currclock + CLOCKS_PER_SEC;
		updwork = 1;

		/* lasketaan ETA */
		eta = ( currclock - beginclock ) * ( psize - bytes ) / ( CLOCKS_PER_SEC * bytes );
		if( eta != curreta )
			updeta = 1;

		if( perc != currperc )
			updperc = 1;
	}

	/* lasketaan sijainti */
	barlen = ( perc * ( UPDBARLEN - 1 ) + 50 ) / 100;
	if( barlen > currbarlen )
	{
		currbarlev = 0;
		updbar = 1;
	}

	/* lasketaan taso */
	avg = AvgLevel( sample, Data.OutSFrequency );
	if( avg == FP0 )
		barlev = 0;
	else if( r_cmpable( avg ) > r_cmpable( FP1 ))
		barlev = UPDBARLEVELS + 1;
	else
	{
		barlev = r_trunc( r_mul( avg, r_itof( UPDBARLEVELS ))) + 1;
		if( barlev == UPDBARLEVELS + 1 )
			barlev = UPDBARLEVELS;
	}
	if( barlev > currbarlev )
		updbar = 1;

	/* päivitetään näyttö jos tarvis */
	if( updwork )
	{
		printf( "%c", Working[ currwork ]);
		currwork = ( currwork + 1 ) % 4;
	}
	if( updperc )
	{
		if( !updwork )
			fputs( fwd( "1" ), stdout );
		printf( "%3d", perc > 999 ? 999 : perc );
		currperc = perc;
	}
	if( updeta )
	{
		int min, sec;
		if( !updperc )
		{
			fputs( fwd( "3" ), stdout );
			if( !updwork )
				fputs( fwd( "1" ), stdout );
		}
		min = eta/60;
		sec = eta - min*60;
		printf( fwd( "2" ) "%02d:%02d", min > 99 ? 99 : min, sec > 59 ? 99 : sec );
		curreta = eta;
	}
	if( updbytes )
	{
		if( !updeta )
		{
			fputs( fwd( "7" ), stdout );
			if( !updperc )
			{
				fputs( fwd( "3" ), stdout );
				if( !updwork )
					fputs( fwd( "1" ), stdout );
			}
		}
		printf( fwd( "2" ) "%8ld", bytes > 99999999 ? 99999999 : bytes );
	}
	if( updbar )
	{
		if( !updbytes )
		{
			fputs( fwd( "10" ), stdout );
			if( !updeta )
			{
				fputs( fwd( "7" ), stdout );
				if( !updperc )
				{
					fputs( fwd( "3" ), stdout );
					if( !updwork )
						fputs( fwd( "1" ), stdout );
				}
			}
		}
		printf( fwd( "%d" ) "%c", barlen + 12, Levels[ barlev ]);
		currbarlen = barlen;
		currbarlev = barlev;
	}
	if( updwork || updperc || updeta || updbytes || updbar )
	{
		fputs( "\r", stdout );
		fflush( stdout );
	}
}

/*		void FinalPercent( void );
 *
 *	Lopettaa prosenttilaskurin.
 */

void FinalPercent( void )
{
#ifdef DEBUG
	IFDEBUG( 20 )	/* sTRICTLY DEBUG */
		return;
#endif
	if( !Verbose )
		return;
	if( UpdateSize )
		fputs( UPDSTRINGCLR, stdout );
}

/*		void Analyse( clock_t clock_diff, long length );
 *
 *	Kertoo laskentanopeuden ja tuloksen voimakkuuden käyttäjälle.
 *	clock_diff on laskentaan kulunut aika 1/50-osasekunteina ja length
 *	on tuloksen pituus näytteinä.
 */

void Analyse( clock_t clock_diff, long length )
{
	r_float peak;
	unsigned int ppeak;

#ifdef DEBUG
	DEBUGMSG( 2, "main: calculate samples/second rate" )
#endif
	if( clock_diff != 0L )
	{
		r_float rate = r_itof( length );
		rate = r_div( rate, r_itof( clock_diff ));
		rate = r_mul( rate, r_itof( CLOCKS_PER_SEC ));
		printf( "Average calculation throughput was %ld samples/second.\n",
				r_round( rate ));
	}

#ifdef DEBUG
	DEBUGMSG( 2, "main: calculate optimal gain" )
#endif
	/* kerrotaan signaalin voimakkuudesta */

	peak = PeakLevel();
	ppeak = r_trunc( r_mul( peak, FP1E2 ));
	if( ppeak == 0 )
		puts( "Output was complete silence." );
	else if( ppeak <= 90 )
		printf( "Output used only %d%% of the dynamic range because of too low a gain.\n",
				ppeak );
	else if( ppeak > 100 )
	{
		if( Data.OutFileformat != FILE_FLOAT )
			printf( "Output was clipped %d%% due to too high a gain setting.\n",
					ppeak - 100 );
	}
	else
		ppeak = 0;
	if( ppeak )
	{
		peak = ffp2dB( peak );
#ifdef _DCC
		char txt1[ 4+4 ], txt2[ 4+4 ];
		r_numftoa( txt1, peak, 4 );
		r_numftoa( txt2, r_sub( Data.Gain, peak ), 4 );
		printf(	"Peak level was %s dB; optimal gain setting would have been %s dB.\n",
				txt1, txt2 );
#else
		printf( "Peak level was %.4f dB; optimal gain setting would have been %.4f dB.\n",
					peak, r_sub( Data.Gain, peak ));
#endif
	}
}

int ScrWidth( void )
{
	char *var;
	int width;
	if(( var = getenv( "COLUMNS" )) == NULL )
		return( WIDTH );
	sscanf( var, "%d", &width );
	if( !( width >= MINWIDTH && width <= MAXWIDTH ))
		return( WIDTH );
	return( width );
}

		/* ---		    Loppu		--- */
