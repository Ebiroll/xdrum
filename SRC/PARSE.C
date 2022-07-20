/*
 *	parse.c			an ugly! source file parser
 *
 *	Copyright © 1996 Jarno Sepp‰nen; see 3MU.c for licensing info.
 *
 *	TO DO!
 */

#include <stdio.h>
#include <string.h>

#include "3MU.h"
#include "console.h"
#include "parse.h"
#include "tags.h"
#include "debug.h"

#include "scrio_pr.h"
#include "tags_pro.h"
#include "misc_pro.h"

		/* ---	   Funktioiden prototyypit	--- */

long StartParse( char *buffer, struct Settings *Data, struct Note *Sequ );
long Parse( char *buffer );
long StopParse( void );

static char *EndWord( char **wbeg, char *wend );
static char *StartSeq( char *ptr, char *wbeg );
static char *EndSeq( char **wbeg, char *wend );
static char *EndTag( char **wbeg, char *wend );

static char *ParseWord( char *beg, char *end );
static void PrintLine( char *wbeg, char *wend, char *min );
static void PrintWord( char *beg, char *end );
static char *SkipComment( char *buf );

		/* ---		  Muuttujat		--- */

r_float			NLvalue = FP0;	/* NoteLen:n laskettava arvo			*/
unsigned short	NLtype = 0;		/* NoteLen:n laskettavan arvon tyyppi	*/
BOOL			NLset = 0;		/* NoteLen asetettu						*/

static struct Settings *Par;
static struct Note *	Seq;

/*	tag on luettavan tagin osoite tai NULL (lippu)
 *	seq on 1, jos ollaan lukemassa sekvenssin j‰seni‰
 *	wordbeg on sanan alkuosoitin ja lippu
 *	tagbeg ja tagend ovat inklusiiviset tagin nimen alku- ja loppuosoittimet
 *	ewbeg on edellisen sanan alkuosoitin
 */

static struct Tag *		tag = NULL;
static BOOL				seq;
static char *wordbeg, *ewbeg, *tagbeg, *tagend;

#define MINFILEIDLEN 6
/*static const char		fileID[ MINFILEIDLEN ] = { '#', '!', '3', 'M', 'U', CHAR_LF};*/
						/*	#!3MU<lf>	*/

		/* ---		   Funktiot		--- */

/*		long StartParse( char *buffer, struct Settings *Data, struct Note *Sequ);
 *
 *	Aloittaa l‰hdekoodin tulkkauksen. buffer osoittaa l‰hdekoodin alkuun ja
 *	sis‰lt‰‰ v‰hint‰‰n MINFILEIDLEN merkki‰ (#!3MU<lf>). Data osoittaa globaaleihin
 *	muuttujiin ja Sequ sekvenssiin.
 *	Data ja Sequ ovat osoittimia tietueisiin, joiden arvoja muutetaan
 *	bufferissa olevan tekstin mukaan.
 *	Palauttaa virhekoodin
 */

long StartParse( char *buffer, struct Settings *Data, struct Note *Sequ )
{
	int ok = 1;

	if( buffer == NULL || Data == NULL || Sequ == NULL )
		return( -1 );

	Par = Data;
	Seq = Sequ;
	tag = NULL;
	seq = 0;
	wordbeg = NULL;
	ewbeg = NULL;
	tagbeg = tagend = NULL;
	NLset = 0;
	Par->Length = 0;

	/* otetaan huomioon myˆs NULLit */
	if( strstr( buffer, "3MU" ) > strchr( buffer, '\n' )
		|| strstr( buffer, "3MU" ) == NULL )
	{
		fputs( "Not a 3MU source file.\n", stderr );
		return( 50 );
	}
	buffer = strchr( buffer, '\n' ) + 1;
	return( Parse( buffer ));
}

/*		long Parse( char *buffer )
 *
 *	buffer osoittaa nollaan p‰‰ttyv‰‰n puskuriin, jossa on ascii-teksti‰.
 *	buffer voi osoittaa joko koko
 *	l‰hdekoodiin tai sitten osaan siit‰ (yksi rivi) siit‰ ja Parse():a
 *	voidaan kutsua monta kertaa kunhan vain antaa joka kerta eri osan (rivin)
 *	k‰sitelt‰v‰ksi ja buffer alkaa aina rivin alusta ja loppuu rivin loppuun.
 *	Tulkinnan p‰‰tteeksi pit‰‰ kutsua StopParse()-funktiota.
 *	Palauttaa `virhekoodin' tai nollan, jos mit‰‰n virhett‰ ei tapahtunut
 *	tekstin formaatti: ks. txt/source_chars.txt ja txt/tags.txt
 */

long Parse( char *buffer )
{
	static BOOL sijoitus = 0; /* oliko sanan j‰lkeen '='-merkki‰ vai ei */
	char *bufptr;
	char *err = NULL;
	BOOL done;
	BOOL quoted = 0;

	if( buffer == NULL )
		return( -1 );

	bufptr = buffer;
	done = !(BOOL)*bufptr;
	bufptr--;

	while( !done && err == NULL )
	{
		bufptr++;
		if( quoted )
		{
			if( *bufptr == CHAR_NUL )
				done = 1;
			if( *bufptr == CHAR_NUL || *bufptr == CHAR_LF )
				err = "Expected a `\"'.";
			else if( *bufptr == CHAR_QUOTE )
			{
				quoted = 0;
				err = EndWord( &wordbeg, bufptr - 1 );
			}
		}
		else switch( *bufptr )
		{
			case CHAR_COMMENT:
				err = EndWord( &wordbeg, bufptr - 1 );
				bufptr = SkipComment( bufptr );
				break;
			case CHAR_NUL:
				done = 1;
			case CHAR_LF:
			case CHAR_TAB:
			case CHAR_SPC:
				err = EndWord( &wordbeg, bufptr - 1 );
				break;
			case CHAR_SEQBEG:
				sijoitus = 0;
				err = StartSeq( bufptr, wordbeg );
				break;
			case CHAR_SEQEND:
				err = EndSeq( &wordbeg, bufptr - 1 );
				break;
			case CHAR_TAGEND:
				sijoitus = 1;
				err = EndTag( &wordbeg, bufptr - 1 );
				break;
			case CHAR_QUOTE:
				if( !sijoitus )	/* tagbeg == NULL || tagend == NULL */
				{
					err = "Tags can't be quoted.";
					break;
				}
				if( tagbeg != NULL && tagend != NULL && !sijoitus )
				{
					/* jaahas, se on sitten toggle */
					tag = ParseTagWithin( tagbeg, tagend );
					tagbeg = tagend = NULL;
					if( tag == NULL )
					{
						err = "Illegal tag name.";
						break;
					}
					if( tag->Type != VAR_TOGGLE )
					{
						err = "Not a toggle.";
						break;
					}
					if( SetTag( tag, NULL ))
					{
						err = "Value not parsed.";
						break;
					}
					tag = NULL;
				}
				sijoitus = 0;
				quoted = 1;
#ifdef DEBUG
				DEBUGMSG( 10, "parse: quoted start of word" )
#endif
				wordbeg = bufptr + 1;
				break;
			default:
				if( wordbeg == NULL )
				{
#ifdef DEBUG
					DEBUGMSG( 10, "parse: start of word" )
#endif
					if( tagbeg != NULL && tagend != NULL && !sijoitus )
					{
						tag = ParseTagWithin( tagbeg, tagend );
						tagbeg = tagend = NULL;
						if( tag == NULL )
						{
							err = "Illegal tag name.";
							break;
						}
						if( tag->Type != VAR_TOGGLE )
						{
							err = "Not a toggle.";
							break;
						}
						if( SetTag( tag, NULL ))
						{
							err = "Value not parsed.";
							break;
						}
						tag = NULL;
					}
					sijoitus = 0;
					wordbeg = bufptr;
					ewbeg = bufptr;
				}
				break;
		}
	}
	if( err != NULL )
	{
		if( Verbose )
		{
			puts( "Error in source on the following line:" );
			PrintLine( ewbeg, bufptr, buffer );
			puts( err );
		}
		return( -1 );
	}
	return( 0 );
}

/*		long StopParse( void );
 *
 *	Lopettaa l‰hdekoodin tulkitsemisen. Palauttaa virhekoodin.
 */

long StopParse( void )
{
	char *err = NULL;
	char errs[200];
	if( tag != NULL )
	{
		sprintf( errs, "No value specified for tag %s.", tag->Label );
		err = errs;
		tag = NULL;
	}
	if( seq && err == NULL )
	{
		err = "Missing `}'.";
		seq = 0;
	}
	if( NLset && err == NULL )
		switch( NLtype )
	{
		case T_NOTE_LEN:
			if( NLvalue == FP0 )
			{
				err = "Zero note sample length.";
				break;
			}
			Par->NoteLen = NLvalue;
			break;
		case T_NOTE_DUR:
			if( Par->OutSFrequency == FP0 )
			{
				err = "Zero output sample frequency.";
				break;
			}
			if( NLvalue == FP0 )
			{
				err = "Zero note duration.";
				break;
			}
			Par->NoteLen = r_mul( NLvalue, Par->OutSFrequency );
			break;
		case T_SEQ_DUR:
			if( Par->OutSFrequency == FP0 )
			{
				err = "Zero output sample frequency.";
				break;
			}
			if( !Par->Length )
			{
				err = "Zero sequence data length.";
				break;
			}
			if( NLvalue == FP0 )
			{
				err = "Zero sequence duration.";
				break;
			}
			Par->NoteLen = r_div( r_mul( NLvalue, Par->OutSFrequency ),
										r_itof( Par->Length ));
			break;
		case T_SEQ_LEN:
			if( !Par->Length )
			{
				err = "Zero sequence data length.";
				break;
			}
			if( NLvalue == FP0 )
			{
				err = "Zero sequence sample length.";
				break;
			}
			Par->NoteLen = r_div( NLvalue, r_itof( Par->Length ));
			break;
		case T_TEMPO:
			if( Par->OutSFrequency == FP0 )
			{
				err = "Zero output sample frequency.";
				break;
			}
			if( NLvalue == FP0 )
			{
				err = "Zero tempo.";
				break;
			}
			Par->NoteLen = r_mul( r_div( Par->OutSFrequency, NLvalue ),
									r_itof( 15 ));
			break;
	}
	NLset = 0;
	if( err != NULL )
	{
		fprintf( stderr, "Error in source:\n%s\n", err );
		return( -1 );
	}
	return( 0 );
}


/*		char *EndWord( char **wbeg, char *wend );
 *
 *	Parserin osa, joka k‰sittelee sanan (whitespacen keskell‰ oleva merkkijono)
 *	lopun eli k‰yt‰nnˆss‰ kutsuu sanan tulkkausfunktiota ParseWord
 *	wbeg on sanan ensimm‰isen merkin osoittimen osoitin ja wend sanan
 *	viimeisen merkin osoitin. Palauttaa virhemerkkijonon osoitteen tai NULL:n.
 */

static char *EndWord( char **wbeg, char *wend )
{
	char *err = NULL;

	if( wbeg == NULL || wend == NULL )
		return( "bug" );

	if( *wbeg != NULL )
	{
#ifdef DEBUG
		DEBUGMSG( 10, "parse: end of word" )
#endif

		if( tag != NULL )
			err = ParseWord( *wbeg, wend );			/* tagin arvo */
		else
		{
			if( tagbeg != NULL || tagend != NULL )
				err = "Syntax error, expected a '='.";	/* CHAR_TAGEND */
			tagbeg = *wbeg;
			tagend = wend;
		}
		*wbeg = NULL;
	}
	return( err );
}

/*		char *StartSeq( char *ptr, char *wbeg );
 *
 *	Parserin osa, joka k‰sittelee sekvenssin { a b c d e ... }
 *	alun eli asettaa muuttujat sekvenssin lukemista varten.
 *	ptr on osoitin sekvenssin alkuun eli '{'-merkkiin, wbeg osoittaa
 *	sanan alkuun; pit‰‰ olla NULL, muuten virhe
 *	Palauttaa virhemerkkijonon.
 */

static char *StartSeq( char *ptr, char *wbeg )
{
	if( ptr == NULL )
		return( "bug" );

	if( tag == NULL || wbeg != NULL )
		return( "Badly placed `{'." );
	if( seq )
		return( "Too many `{'s." );

#ifdef DEBUG
	DEBUGMSG( 10, "parse: start of sequence" )
#endif

	seq = 1;
	return( NULL );
}

/*		char *EndSeq( char **wbeg, char *wend );
 *
 *	Parserin osa, joka k‰sittelee sekvenssin { a b c d e ... }
 *	lopun eli k‰sittelee mahdollisen sekvenssin viimeisen sanan
 *	ja lopettaa sekvenssin lukemisen (asettaa muuttujat siten).
 *	wbeg on mahd. sekvenssin viimeisen sanan osoittimen osoite ja
 *	wend on sanan viimeisen merkin osoite.
 *	T‰m‰ rutiini tulkkaa sen viimeisen sanan tilanteessa, jossa sanan ja
 *	sekvenssin loppumerkin '}' v‰lill‰ ei ole whitespacea eik‰ siten
 *	EndWord ole tulkinnut t‰t‰ sekvenssin viimeist‰ sanaa.
 *	Palauttaa virhemerkkijonon.
 */

static char *EndSeq( char **wbeg, char *wend )
{
	char *err = NULL;

	if( wbeg == NULL || wend == NULL)
		return( "bug" );

	if( tag == NULL )
		return( "Badly placed `}'." );
	if( !seq )
		return( "Missing `{'." );

#ifdef DEBUG
	DEBUGMSG( 10, "parse: end of sequence" )
#endif

	if( *wbeg != NULL )
	{
		/* viimeinen j‰sen=alkio on viel‰ k‰sittelem‰tt‰ */

		err = ParseWord( *wbeg, wend );
		*wbeg = NULL;
	}

	/* sekvenssin ja tagin arvojen luku loppuu */

	seq = 0;
	tag = NULL;
	if( err != NULL )
		return( err );

	return( NULL );
}

/*		char *EndTag( char **wbeg, char *wend );
 *
 *	Parserin osa, joka k‰sittelee ns. tagin eli muutettavan arvon nimen
 *	lopun. Esimerkki tagista on "Output-Frequency". Kutsuu ParseTagWithin:a
 *	joka tulkitsee tag-tekstin, ja asettaa sen palauttaman tag-osoittimen
 *	muuttujaan, jotta tagia seuraava arvo laitetaan tagin implikoimaan
 *	osoitteeseen.
 *	*wbeg on tag-sanan ensimm‰isen ja wend viimeisen merkin osoite
 *	Palauttaa virheilmoituksen.
 */

static char *EndTag( char **wbeg, char *wend )
{
	if( wbeg == NULL || wend == NULL )
		return( "bug" );

	if( seq || tag != NULL )
	{
		tag = NULL;
		return( "Badly placed `='." );	/* CHAR_TAGEND */
	}
#ifdef DEBUG
	DEBUGMSG( 10, "parse: end of tag" )
#endif

	if( tagbeg == NULL || tagend == NULL )
	{
		tag = ParseTagWithin( *wbeg, wend );
		*wbeg = NULL;
	}
	else
		tag = ParseTagWithin( tagbeg, tagend );
	tagbeg = tagend = NULL;

	if( tag == NULL )
		return( "Illegal tag name." );

	return( NULL );
}

/*		char *ParseWord( char *beg, char *end );
 *
 *	Tulkitsee v‰lille [beg, end] kuuluvan sanan eli beg ja end ovat inklusiivisia.
 *	Kutsuu SetTag()a ja ottaa huomioon sekvenssin (seq) yms. muuttujat.
 *	Palauttaa virheilmoituksen tai NULLin.
 */

static char *ParseWord( char *beg, char *end )
{
	char t;
	char *err = NULL;

	if( beg == NULL || end == NULL )
		return( "bug" );

#ifdef DEBUG
	DEBUGMSG( 10, "parse: parse word" )
#endif

	/* tehd‰‰n sanasta v‰liaikaisesti nollaan p‰‰ttyv‰ */
	t = *( end + 1 );
	*( end + 1 ) = 0;

	if( tag != NULL )
	{
		if( SetTag( tag, beg ))
			err = "Value not parsed.";
		if( !seq )
			tag = NULL;
	}
	else
		err = "Legendary syntax error (expected a tag).";

	/* palautetaan nollaksi muutettu merkki takaisin */
	*( end + 1 ) = t;

	return( err );
}

/*		void PrintLine( char *wbeg, char *wend, char *min );
 *
 *	Funktio, joka tulostaa rivin, jolla sana [wbeg, wend] sijaitsee siten,
 *	ett‰ sana alleviivataan. min on rivin alun minimiparametri, ts. min:n
 *	alle ei menn‰ rivin alkua etsitt‰ess‰.
 */

static void PrintLine( char *wbeg, char *wend, char *min )
{
	char *lbeg, *lend;	/* tulostettavan rivin alku ja loppu */

	if( !Verbose || min == NULL || ( wbeg == NULL && wend == NULL ))
		return;
	if( wbeg == NULL )
		wbeg = wend;
	if( wend == NULL )
		wend = wbeg;

	if( !*wend )
		return;

	lbeg = wbeg - 1;
	while( *lbeg != CHAR_LF && lbeg >= min )
		lbeg--;
	lbeg++;

	lend = wend + 1;
	while( *lend != CHAR_LF && *lend )
		lend++;

	PrintWord( lbeg, wbeg );
	printf( ULON );
	PrintWord( wbeg, wend + 1 );
	printf( ULOFF );
	PrintWord( wend + 1, lend );
	putchar( CHAR_LF );
}

/*		void PrintWord( char *beg, char *end );
 *
 *		PrintLine():n apufunktio, joka tulostaa tekstin v‰lilt‰ [beg, end[.
 *	Huom! end on eksklusiivinen
 */

static void PrintWord( char *beg, char *end )
{
	char t = *end;
	*end = 0;
	printf( "%s", beg );
	*end = t;
}

/*		char *SkipComment( char *buf );
 *
 *	Hypp‰‰ buf:n osoittamassa kohdassa mahd. olevan kommentin yli
 *	ja palauttaa osoittimen kommenttia seuraavaan merkkiin.
 */

static char *SkipComment( char *buf )
{
	if( buf == NULL )
		return( NULL );

#ifdef DEBUG
	DEBUGMSG( 10, "parse: skipping comment" )
#endif
	if( *buf == CHAR_COMMENT )
		while( *buf != CHAR_LF )
			buf++;
	return( buf );
}

		/* ---		    Loppu		--- */
