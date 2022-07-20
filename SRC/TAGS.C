/*
 *	tags.c			source tag related functions
 *
 *	Copyright © 1996 Jarno Sepp‰nen; see 3MU.c for licensing info.
 */

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "3MU.h"
#include "console.h"
#include "r_ffp/r_ffp.h"
#include "tags.h"
#include "debug.h"
#include "misc_pro.h"
#include "parse_pr.h"
#include "r_ffp/r_ffp_pr.h"
#include "scrio_pr.h"

		/* ---	Ulkoisten muuttujien esittelyt	--- */

/* taglist.c */
extern struct Tag Tags[];

		/* ---	   Funktioiden prototyypit	--- */

int DefaultTags( const char *srcfilename );
void PrintTags( BOOL defaults, BOOL header );
int PrintTag( char *tag );

struct Tag *ParseTagWithin( char *beg, char *end );
struct Tag *ParseTag( const char *name );
int SetTagName( const char *name, const char *value );
int SetTagNum( unsigned short num, const char *value );
int SetTag( struct Tag *tag, const char *value );
static int DefaultNote( struct Note *n );
static int LowSetTag( struct Tag *tag, const char *value, struct Note *seq,
						unsigned short seqnum );

		/* ---		  Muuttujat		--- */

static struct Note Default;


		/* ---		   Funktiot		--- */

/*		int DefaultTags( const char *srcfilename )
 *
 *	Asettaa tageille oletusarvot Tag->Default-kent‰st‰.
 *	Kaikki [sekventiaaliset?] arvot, joilla ei ole oletusarvoa
 *	j‰tet‰‰n ennalleen (nollaksi).
 *	Output-Filename asetetaan srcfilename:sta johdetuksi tekstiksi.
 *	Palauttaa nollan jos kaikki meni hyvin, muuten palauttaa arvon tosi.
 */

int DefaultTags( const char *srcfilename )
{
	struct Tag *t;
	int i, sl;
	char outname[ FILENAME_MAX ];

	if( srcfilename != NULL )
	{
		/* asetetaan l‰htˆtiedoston nimeksi l‰hdekooditiedoston nimi, jonka
		   tarkennin on korvattu .snd:ll‰ */
		sl = strlen( srcfilename );
		strncpy( outname, srcfilename, FILENAME_MAX );
		if(( sl >= 4 ) && !strcmp( &srcfilename[ sl - 4 ], ".3MU" ))
			outname[ sl - 4 ] = 0;
		strncat( outname, ".snd", FILENAME_MAX );
		SetTextVar( &Data.OutFilename, outname );
	}
	
	/* asetetaan muut oletusarvot */
	for( i = 1; strcmp(( t = &Tags[ i ])->Label, END_TAG ); i++ )
		if( t->Default != NULL )
			if( LowSetTag( t, t->Default, &Default, 0 ))
				return( 1 );
	return( 0 );
}

/*		void PrintTags( BOOL defaults, BOOL header );
 *
 *	Tulostaa listan kaikkien tagien nimist‰.
 *	Jos defaults!=0 tulostaa myˆs oletusarvot ja jos header!=0
 *	tulostaa alkuun otsikon ja loppuun tagien lukum‰‰r‰n.
 */

/* This-Is-A-Tag-W/Default (200,6), */
/* This-Is-A-Sequential-Tag-Or-A-Tag-W/O-Default, */

void PrintTags( BOOL defaults, BOOL header )
{
	struct Tag *ptr = &Tags[ 1 ];
	int i, len = 0;

	if( !Verbose )
		return;
	if( header )
		printf( ULON "Recognized tags%s:" ULOFF "\n",
				defaults ? " (default values in parentheses)" : "" );

	for( i = 1; strcmp( ptr->Label, END_TAG ); i++ )
	{
		len += strlen( ptr->Label );
		if( defaults )
			if( ptr->Default != NULL )
				len += strlen( ptr->Default ) + 3;
			else if( ptr->Sequential )
				len += 4;
		if( len >= ScrWidth() - 1 && strlen( ptr->Label ) < ScrWidth())
		{
			putchar( '\n' );
			len = strlen( ptr->Label );
			if( defaults )
				if( ptr->Default != NULL )
					len += strlen( ptr->Default ) + 3;
				else if( ptr->Sequential )
					len += 4;
		}
		printf( "%s", ptr->Label );
		if( defaults )
			if( ptr->Default != NULL )
				printf( " (%s)", ptr->Default );
			else if( ptr->Sequential )
				printf( " (0)" );
		ptr++;
		if( strcmp( ptr->Label, END_TAG ))
		{
			printf( ", " );
			len += 2;
		}
	}
	puts( "\n" );
	if( header )
		printf( "Total of %d tags.\n", i - 1 );
}

/*		int PrintTag( char *tag );
 *
 *	Tulostaa tag:n osoittaman tekstin nimisen tagin arvon.
 *	Palauttaa nollan, jos kaikki meni OK.
 *	Ei printtaa sekvenssien eik‰ VAR_CUSTOM-tagien arvoja.
 */

int PrintTag( char *tag )
{
	struct Tag *t;
#ifdef _DCC
	char des[ 4 ];
	long kok;
#endif

	if( !Verbose )
		return( 0 );

	if(( t = ParseTag( tag )) == NULL )
	{
		fprintf( stderr, "Illegal tag name `%s'.\n", tag );
		return( 1 );
	}
	if( t->Sequential )
	{
		fputs( "Print: Sequential tags are not supported (maybe someday).\n", stderr );
		return( 1 );
	}
	printf( "Value of tag %s is ", t->Label );
	switch( t->Type )
	{
		case VAR_TEXT:
			puts( *( (char **)t->Location.Variable ));
			return( 0 );
		case VAR_UWORD:
			printf( "%ld\n", (long)*( (unsigned short *)t->Location.Variable ));
			return( 0 );
		case VAR_UBYTE:
			printf( "%ld\n", (long)*( (unsigned char *)t->Location.Variable ));
			return( 0 );
		case VAR_FLOAT:
#ifdef _DCC
			kok = r_rndftoa( *( (r_float *)t->Location.Variable ), des, 3 );
			printf( "%ld.%s\n", kok, des );
#else
			printf( "%.3f\n", *( (r_float *)t->Location.Variable ));
#endif
			return( 0 );
		case VAR_NLEN:
		case VAR_FREQ:
		case VAR_CUSTOM:
			fputs( "I cannot print this value--maybe someday\n", stderr );
			return( 1 );
		case VAR_TOGGLE:
			puts( *( (char *)t->Location.Variable ) ? "on" : "off" );
			return( 0 );
	}
	fputs( "Bug: Unknown tag number.\n", stderr );
	return( 1 );
}

/*		struct Tag *ParseTagWithin( char *beg, char *end )
 *
 *	Tulkitsee v‰lill‰ [beg, end] olevan tekstin tagin nimeksi.
 *	Mik‰li tekstin tagi on virheellinen palautetaan NULL, muuten
 *	palautetaan kyseisen tagin osoite.
 */

struct Tag *ParseTagWithin( char *beg, char *end )
{
	char t;
	struct Tag *tag;

	if( beg == NULL || end == NULL )
		return( NULL );

	t = *(end+1);
	*(end+1) = 0;
	tag = ParseTag( beg );
	*(end+1) = t;
	return( tag );
}

/*		struct Tag *ParseTag( const char *name );
 *
 *	Tulkitsee name:n osoittaman tekstin tagin nimeksi.
 *	Mik‰li tekstin tagi on virheellinen palautetaan NULL, muuten
 *	palautetaan kyseisen tagin osoite.
 */

struct Tag *ParseTag( const char *name )
{
	BOOL found = 0;
	int i;

	if( name == NULL )
		return( NULL );

#ifdef DEBUG
	DEBUGMSG( 9, "parse: parse tag:" )
	DEBUGMSG( 9, name )
#endif

	for( i = 0; strcmp( Tags[ i ].Label, END_TAG ); i++ )
		if( CmpS( name, Tags[ i ].Label ))
		{
			found = 1;
			break;
		}

	return( found ? &Tags[ i ] : NULL );
}

/*		int SetTagName( const char *name, const char *value );
 *
 *	Asettaa tag:n osoittaman tekstin nimisen tagin value:n osoittaman
 *	tekstin arvoon. Palautuu ParseTag()- ja SetTag()-kutsuiksi.
 *	Jos value==NULL, antaa tag-muuttujalle arvon NULL.
 *	Palauttaa nollan jos kaikki OK.
 */

int SetTagName( const char *name, const char *value )
{
	struct Tag *tag;
	if(( tag = ParseTag( name )) == NULL )
	{
		fprintf( stderr, "Unknown tag name `%s'.\n", name );
		return( 1 );
	}

	return( SetTag( tag, value ));
}

/*		int SetTagNum( unsigned short num, const char *value );
 *
 *	Asettaa tagille numero num arvon merkkijonosta value.
 *	Jos value==NULL, antaa tag-muuttujalle arvon NULL.
 *	Palauttaa nollan jos kaikki OK. Palautuu SetTag():ksi.
 */

int SetTagNum( unsigned short num, const char *value )
{
	struct Tag *t = NULL;
	int i;
	/* etsit‰‰n num-numeroinen tag */

	for( i = 1; Tags[ i ].Number != T_NULL; i++ )
		if( Tags[ i ].Number == num )
		{
			t = &Tags[ i ];
			break;
		}
	if( t == NULL )
	{
		fprintf( stderr, "Illegal tag number %d.\n", num );
		return( 1 );
	}

	return( SetTag( t, value ));
}

/*		int SetTag( struct Tag *tag, const char *value );
 *
 *	Asettaa tagille tag arvon merkkijonosta value.
 *	Jos value==NULL, antaa tag-muuttujalle arvon NULL.
 *	Palauttaa nollan jos kaikki OK. Palautuu LowSetTag():ksi.
 */

int SetTag( struct Tag *tag, const char *value )
{
	static int			SeqNum = 0;		/* sekvenssin alkion numero	*/
	static struct Tag *	SeqCurr;		/* sekvenssitagi			*/
	static BOOL			InitSeqLen;		/* ekan tagin aikana 1		*/

	if( tag == NULL )
	{
		fputs( "bug\n", stderr );
		return( 1 );
	}

	if( SeqCurr != NULL && SeqCurr != tag )
	{
		/* sekvenssin loppu */
		InitSeqLen = 0;
		SeqCurr = NULL;
		if( SeqNum != Data.Length )
		{
			if( SeqNum < Data.Length )
				fputs( "Too few notes in sequence.\n", stderr );
			else 
				fputs( "Too many notes in sequence.\n", stderr );
			return( 1 );
		}
	}
	if( tag->Sequential )
	{
		if( Data.Length == 0 )
		{
			InitSeqLen = 1;		/* eka tagi			*/
			SeqCurr = NULL;
		}
		if( SeqCurr == NULL )	/* uusi sekvenssi	*/
		{
			/* uusi sekvenssi */
			SeqNum = 0;
			SeqCurr = tag;
		}
		SeqNum++;
		if( SeqNum > MAXNOTES )
		{
			fputs( "Too many notes in sequence.\n", stderr );
			return( 1 );
		}
		if( InitSeqLen )
		{
			Data.Length = SeqNum;
			if( DefaultNote( &Sequence[ SeqNum - 1 ] ))
				return( 1 );
		}
		return( LowSetTag( tag, value, Sequence, SeqNum - 1 ));
	}
	return( LowSetTag( tag, value, NULL, 0 ));
}

/*		int LowSetTag( struct Tag *tag, const char *value, struct Note *seq,
 *						unsigned short seqnum );
 *
 *	Tulkitsee valuen osoittaman tekstin tag-muuttujan arvoksi ja kopioi
 *	arvon oikeaan muistipaikkaan. Jos tag on sekvenssimuuttuja, luetaan
 *	sekvenssin j‰senen numero seqnum:sta. Muuttujan arvon sijoituspaikka
 *	saadaan tag-tietueesta (ja mahd. seqnum:sta).
 *	seq on *sekvenssin* osoite; voi olla NULL, jos tagi ei ole sekventiaalinen.
 *	Jos value==NULL, antaa tag-muuttujalle arvon NULL.
 *	Palauttaa virhelipun.
 */

static int LowSetTag( struct Tag *tag, const char *value, struct Note *seq,
						unsigned short seqnum )
{
	void *varptr;	/* tiedon tallennuspaikan osoitin */
	char *err;

	if( tag == NULL )
	{
		fputs( "bug\n", stderr );
		return( 1 );
	}

#ifdef DEBUG
	DEBUGMSG( 9, "parse: parse value:" )
#endif

	/* asetetaan varptr */

	if( tag->Sequential )
	{
		if( seq == NULL )
			return( 1 );

		varptr = (void *)((unsigned long)( &seq[ seqnum ]) + 
							tag->Location.Offset );
#ifdef DEBUG
		IFDEBUG(6)
			fprintf( stderr,
				"var|Sekvenssinro:%d, Offset:%ld => Sekvenssioffset:%ld\n",
				seqnum, (long)tag->Location.Offset, (long)varptr - (long)seq );
#endif
	}
	else
	{
		varptr = tag->Location.Variable;
#ifdef DEBUG
		IFDEBUG(6)
		{
			int off = (long)( tag->Location.Variable ) - (long)&Data;
			if(( off >= 0 ) && ( off <= sizeof( struct Settings )))
				fprintf( stderr, "var|Settings-offset:%d\n", off );
			else
				fprintf( stderr, "var|Absoluuttinen osoite:%08lx\n",
							(unsigned long)tag->Location.Variable );
		}
#endif
	}

#ifdef DEBUG
	IFDEBUG( 9 )
		if( value )
		{
			fputs( value, stderr );
			fputc( '\n', stderr );
		}
#endif

	/* tulkataan arvo ja sijoitetaan *varptr:iin */
	err = InterpTagType( tag, value, varptr );

	if( err )
	{
		fputs( err, stderr );
		fputc( '\n', stderr );
		return( 1 );
	}
	return( 0 );
}

/*		int DefaultNote( struct Note *n )
 *
 *	Asettaa oletusarvot nuotille n.
 *	Palauttaa nollan jos kaikki meni hyvin, muuten palauttaa arvon tosi.
 */

static int DefaultNote( struct Note *n )
{
	memcpy( (void *)n, (void *)&Default, sizeof( struct Note ));
	return( 0 );
}

		/* ---		    Loppu		--- */
