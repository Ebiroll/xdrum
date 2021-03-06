/*
 *	args.c			command line argument parser
 *
 *	Copyright � 1996 Jarno Sepp�nen; see 3MU.c for licensing info.
 */

#include <stdio.h>
#include <string.h>
#include "3MU.h"
#include "opts.h"
#include "tags.h"
#include "debug.h"

#include "io_proto.h"
#include "misc_pro.h"
#include "parse_pr.h"
#include "scrio_pr.h"
#include "tags_pro.h"
#include "seq_prot.h"

		/* ---	   Funktioiden prototyypit	--- */

void ParseEarlyArgs( int argc, char *argv[] );
BOOL ParseArgs( int argc, char *argv[], char **src );
static int ParseOpt( int o, char *next, int *nextused, BOOL nextarg,
						BOOL delayed, BOOL *srcselected );
static int ParseDelayedOpt( int o, char *next, int *nextused, BOOL nextarg );


		/* ---		  Muuttujat		--- */

/* ks. n�iden kutsumanimet opts.h:sta */
static char *Opts[ NUM_OPTS ] =
{
	"b", "if", "i", "it", "j", "of", "o", "ot",
	"q", "s", "t", "v", "O"
#ifdef DEBUG
	,"d"
#endif
};

		/* ---		   Funktiot		--- */

/*		void ParseEarlyArgs( int argc, char *argv[] );
 *
 *	argc ja argv ovat normaalit C-ohjelman argumenttimuuttujat.
 *	T�m� funktio k�y argumentit l�pi ja jos se l�yt�� option
 *	QUIETOPT tai STDOUTOPT ilman parametria niin se nollaa muuttujan Verbose.
 *	Etsii my�s DEBUGOPT:n
 */

void ParseEarlyArgs( int argc, char *argv[] )
{
	int n;
	Verbose = 1;
	for( n = 1; n < argc; n++ )
	{
		if(	( !strcmp( argv[ n ], QUIETOPT )) ||
			( !strcmp( argv[ n ], STDOUTOPT ) &&
				(( n + 1 == argc ) || ( *argv[ n + 1 ] == '-' ))
			))
			Verbose = 0;
#ifdef DEBUG
		else if( !strcmp( argv[ n ], DEBUGOPT ))
		{
			DebugLevel = 1;
			if(( n + 1 != argc ) && ( *argv[ n + 1 ] != '-' ))
			{
				sscanf( argv[ n + 1 ], "%d", &DebugLevel );
				n++;
			}
			IFDEBUG(1)
				fprintf( stderr,
					"Debugging enabled at level %d. Have a nice day.\n",
					DebugLevel );
		}
#endif
	}
}

/*		BOOL ParseArgs( int argc, char *argv[], char **src );
 *
 *	argc ja argv ovat C-ohjelman argumenttimuuttujat. src on joko NULL
 *	tai osoite, johon laitetaan l�hdekooditiedoston nimen osoite.
 *	Jos optio -s valitaan, niin ParseArgs laittaa *src=NULL.
 *	Jos src==NULL, k�sitell��n vain ns. viiv�stetyt optiot, eli optiot,
 *	joita ennen pit�� olla mm. l�hdekoodi tulkittu ja tagien arvot asetettu.
 *	Palauttaa 1:n, jos 3MU:sta pit�� poistua ParseArgs():n kutsumisen j�lkeen.
 *	T�ll�isi� tapauksia ovat mm. optioiden -v ja -t k�ytt�; ne tulostavat
 *	tietoja jonka j�lkeen 3MU lopettaa. My�s virheellinen optioiden k�ytt�
 *	saattaa aiheuttaa ohjelman suorituksen keskeytymisen.
 */

BOOL ParseArgs( int argc, char *argv[], char **src )
{
	BOOL quit = 0, src_stdin = 0;
	int incr, n;
	for( n = 1; n < argc; n++ )
	{
		if( *argv[ n ] == '-' )
		{
			int o;
			BOOL tquit;
			for( o = 0; o < NUM_OPTS; o++ )
				if( !strcmp( argv[ n ] + 1, Opts[ o ]))
					break;
			if( o == NUM_OPTS )
			{
				fprintf( stderr, "Unknown option `%s'.\n", argv[ n ]);
				quit = 1;
				continue;
			}
			incr = 0;
			if( n + 1 < argc )
			{
				char *tmp = argv[ n + 1 ];
				tquit = ParseOpt( o, tmp, &incr, *tmp == '-', src == NULL,
									&src_stdin );
			}
			else
				tquit = ParseOpt( o, (char *)0L, &incr, 0, src == NULL,
									&src_stdin );
			quit = quit || tquit;
			n += incr;
		}
		/* ei optio; tiedostonnimi: */
		else if( src != NULL )
		{
			if( !src_stdin && *src == NULL )
				*src = argv[ n ];
			else
			{
				fprintf( stderr, "Extraneous argument `%s'.\n", argv[ n ]);
				quit = 1;
			}
		}
	}
	/* optio -s */
	if( src != NULL )
	{
		if( src_stdin )
			*src = NULL;
		else if( *src == NULL && !quit )
		{
			fputs( "Missing source file name.\n", stderr );
			quit = 1;
		}
	}
	return( quit );
}

/*		int ParseOpt( int o, char *next, int *nextused, BOOL nextarg,
 *						BOOL delayed, BOOL *srcselected );
 *
 *	ParseArgs():n osa, joka tulkitsee yksitt�isen option numero o ja
 *	tekee tarvittavat asiat, esim. tulostaa tietoja tms.
 *	next on osoitin seuraavaan argumenttiin tai NULL, jos k�sittelyss�
 *	on viimeinen argumentti. nextused on osoitin lippuun, joka ilmoittaa,
 *	liittyyk� seuraava argumentti next t�h�n optioon; ParseOpt _asettaa_
 *	*nextused-lipun optiokohtaisesti. Jos ParseOpt asettaa *nextused:n
 *	1:ksi, niin ParseArgs() hypp�� seuraavan argumentin yli, koska se on
 *	jo k�sitelty. Esimerkkin� optiot, jotka vaativat jonkun parametrin, kuten
 *	-o <x>
 *	nextarg on lippu, joka on ylh��ll�=tosi, mik�li seuraava argumentti
 *	on optio eli alkaa '-'-merkill�.
 *	delayed on lippu, joka on tosi silloin, kuin kyseess� viivytetty
 *	optioiden tulkinta. T�ll�in ParseOpt()-kutsu palautuu ParseDelayedOpt()-
 *	kutsuksi.
 *	srcselected on osoitin lippuun, jonka ParseOpt() asettaa, jos l�hde
 *	on valittu optiolla -s.
 *	
 *	Palauttaa 1:n, jos ParseArg():n j�lkeen pit�� poistua suoraan, muuten 0:n.
 */

static int ParseOpt( int o, char *next, int *nextused, BOOL nextarg,
						BOOL delayed, BOOL *srcselected )
{
	static BOOL stdin_used = 0;

	/* nextok ilmoittaa mik�li seuraavana argumenttina on ei-optio */
	BOOL nextok = next != NULL && !nextarg;
	long bs;

	if( delayed )
		return( ParseDelayedOpt( o, next, nextused, nextarg ));

	/*	ensin k�sitell��n optiot, jotka _eiv�t_ k�yt� lis�m��rett�
	 *	esim. -v, -t
	 */
	switch( o )
	{
		/* -v print version */
		case OPT_VERSION:
			printf( "Environment name: `%s' version: <%s>\n"
					"Engine name: `%s' version: <%s>\n",
					EnvironName, EnvironVersion,
					EngineName, EngineVersion );
			return( 1 );

		/* -t print tags */
		case OPT_TAG:
			if( !nextok )
			{
				PrintTags( 1, 1 );	/* sek� oletusarvot ett� otsikko halutaan */
				return( 1 );
			}
			break;	/* delayed processing */

#ifdef DEBUG
		/* -d debug */
		case OPT_DEBUG:
			if( nextok )
				*nextused = 1;
			return( 0 );	/* early processing */
#endif

		/* -q quiet operation */
		case OPT_QUIET:
			return( 0 );	/* early processing */

		/* -s source STDIN */
		case OPT_STDIN:
			if( stdin_used )
			{
				fputs( "Switch -s: STDIN can be used to only one application at a time.\n",
						stderr );
				return( 1 );
			}
			stdin_used = 1;
			*srcselected = 1;
			Query = 0;	/* ei kysyt� mit��n... */
			return( 0 );

		/* -i input from STDIN */
		case OPT_INFILE:
			if( !nextok )
			{
				if( stdin_used )
				{
					fputs( "Switch -i: STDIN can be used to only one application at a time.\n",
							stderr );
					return( 1 );
				}
				stdin_used = 1;
				SetTagNum( T_WAVE_FILEN, NULL );
				Query = 0;	/* ei kysell�... */
				return( 0 );
			}
			break;	/* delayed processing */

		/* -o delayed processing */
		case OPT_OUTFILE:
			if( !nextok )
				return( 0 );
			break;

		case OPT_OVERWR:
			Query = 0;
			return( 0 );
	}
	if( next == NULL )
	{
		fputs( "Missing argument.\n", stderr );
		return( 1 );
	}
	*nextused = 1;

	/*	sitten k�sitell��n optiot, jotka _vaativat_ lis�m��reen
	 *	esim. -b <size>, -t <tag>
	 */
	switch( o )
	{
		/* -b i/o buffer size */
		case OPT_IOBUF:
			sscanf( next, "%ld", &bs );
			if( bs <= 0 )
			{
				fputs( "Illegal I/O buffer size.\n", stderr );
				return( -1 );
			}
			IObufSize = (size_t)bs;
			return( 0 );

		/* -j progress indicator jump */
		case OPT_PIJUMP:
			sscanf( next, "%ld", &UpdateSize );
			return( 0 );			

	/* delayed: */
		case OPT_INFREQ:
		case OPT_INFILE:
		case OPT_INFORM:
		case OPT_OUTFREQ:
		case OPT_OUTFILE:
		case OPT_OUTFORM:
		case OPT_TAG:
			return( 0 );
	}
	*nextused = 0;
	fputs( "Bug: Unknown option number (args).\n", stderr );
	return( 1 );
}

/*		int ParseDelayedOpt( int o, char *next, int *nextused, BOOL nextarg );
 *
 *	ParseArgs():n osa, joka k�sittelee viivytetyt optiot. N�m� ovat:
 *	-t <t>		tagin arvon tulostus
 *	-t <t>=<a>	tagin arvon asetus
 *	-if <t>		tulon taajuuden asetus
 *	-i <n>		tulotiedoston nimen asetus
 *	-it <t>		tulevan datan tyypin asetus
 *	-of <t>		l�hdon taajuuden asetus
 *	-o <n>		l�ht�tiedoston nimen asetus
 *	-ot <t>		l�htev�n datan tyypin asetus
 *
 *	ParseDelayedOpt():n rajapinta on sama kuin ParseOpt():lla.
 */

static int ParseDelayedOpt( int o, char *next, int *nextused, BOOL nextarg )
{
	BOOL nextok = next != NULL && !nextarg;
	if( o == OPT_TAG && nextok )
	{
		BOOL set = 0;
		int i;
		*nextused = 1;
		for( i = 0; next[ i ]; i++ )
			if( next[ i ] == '=' )
			{
				next[ i ] = 0;
				set = 1;
				break;
			}
		if( set )
		{
			if( SetTagName( next, &next[ i + 1 ]))
				return( 1 );
			if( Verbose )
				printf( "Tag %s set to value %s.\n", next, &next[ i + 1 ]);
		}
		else
			if( PrintTag( next ))
				return( 1 );
		return( 0 );
	}
	*nextused = 1;
	switch( o )
	{
		case OPT_INFREQ:
			SetTagNum( T_WAVE_SFREQ, next );
			break;
		case OPT_INFILE:
			SetTagNum( T_WAVE_FILEN, next );
			break;
		case OPT_INFORM:
			SetTagNum( T_WAVE_FILEF, next );
			break;
		case OPT_OUTFREQ:
			SetTagNum( T_OUT_SFREQ, next );
			break;
		case OPT_OUTFILE:
			if( !nextok )
			{
				SetTagNum( T_OUT_FILEN, NULL );	/* output to STDOUT */
				*nextused = 0;
			}
			else
				SetTagNum( T_OUT_FILEN, next );
			break;
		case OPT_OUTFORM:
			SetTagNum( T_OUT_FILEF, next );
			break;
		default:
			*nextused = 0;
			break;
	}
	return( 0 );
}

		/* ---		    Loppu		--- */
