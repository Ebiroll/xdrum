/*
 *	parse2.c		part 2 of 3MU source file parser
 *
 *	Copyright © 1996 Jarno Seppänen; see 3MU.c for licensing info.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "3MU.h"
#include "tags.h"
#include "debug.h"

#include "r_ffp/r_ffp.h"
#include "misc_pro.h"
#include "seq_prot.h"

		/* ---	Ulkoisten muuttujien esittelyt	--- */

extern r_float			NLvalue;
extern unsigned short	NLtype;
extern BOOL				NLset;

		/* ---	   Funktioiden prototyypit	--- */

char *InterpTagType( struct Tag *tag, const char *beg, void *var );
char *SetTextVar( char **var, char *text );

static char *InterpTagNumber( struct Tag *tag, char *beg, void *var);
static int ParseNote( char *asc, r_float *freq, signed char *notenum );

#if 0
static BOOL ParseVersion( char *ver, int *pri, int *sec, char **post );
#endif

		/* ---		   Funktiot		--- */

/*		char *InterpTagType( struct Tag *tag, const char *beg, void *var );
 *
 *	Lukee tagin *tag arvon beg:n osoittamasta tekstistä ja sijoittaa sen
 *	var:n osoittamaan muistipaikkaan. Jos beg==NULL, asettaa *var=NULL.
 *	Vain VAR_TEXT-muuttujan voi asettaa NULL:ksi. Jos tagi on
 *	VAR_TOGGLE ja beg==NULL, kääntää tagin arvon toisinpäin.
 *	Jos tag on 'VAR_CUSTOM'-tagi niin palautuu InterpTagNumber()-kutsuksi.
 *	Palauttaa virheilmoituksen.
 */

char *InterpTagType( struct Tag *tag, const char *beg, void *var )
{
	char *err = NULL, *uusi;
	long temp;
	int result;

#ifdef DEBUG
	DEBUGMSG( 10, "parse2: interpret tag type" )
#endif

	if( tag == NULL || var == NULL )
		return( "bug" );

	if( beg == NULL )
	{
		if( tag->Type != VAR_TEXT && tag->Type != VAR_TOGGLE )
		return( "bug @ parse2.c:InterpTagType() caller" );
	}

	if( beg != NULL )
	{
		/* kopioidaan tulkittava dynaamisesti varattuun muistiin, jolloin saadaan muuttaa sitä */
		if(( uusi = malloc( strlen( beg )+1)) == NULL )
			return( "Not enough memory." );
		strcpy( uusi, beg );
	}
	else
		uusi = NULL;

	switch( tag->Type )
	{
		case VAR_CUSTOM:
			err = InterpTagNumber( tag, uusi, var );
			break;

		case VAR_FLOAT:
			Comma2Period( uusi );
			*((r_float *)var) = afp( uusi );
			break;

		case VAR_TEXT:
			err = SetTextVar( (char **)var, uusi );
			break;

		case VAR_UBYTE:
			sscanf( uusi, "%ld", &temp );
			*((unsigned char *)var) = (unsigned char)temp;
			break;

		case VAR_UWORD:
			sscanf( uusi, "%ld", &temp );
			*((unsigned short *)var) = (unsigned short)temp;
			break;

		case VAR_TOGGLE:
			if( uusi == NULL )
				*( (char *)var ) = *( (char *)var ) == 0;
			else
			{
				if( CmpS( uusi, "on" ) || CmpS( uusi, "1" ))
					*( (char *)var ) = 1;
				else if( CmpS( uusi, "off" ) || CmpS( uusi, "0" ))
					*( (char *)var ) = 0;
				else
					err = "Unknown toggle value.";
			}
			break;

		case VAR_NLEN:
			if( NLset )
			{
				err =	"Multiple definitions of sequence length.\n"
						"Tags Note-Duration, Note-Sample-Duration, Sequence-Duration,\n"
						"Sequence-Sample-Duration and Tempo are mutually exclusive.";
				break;
			}
			Comma2Period( uusi );
			*((r_float *)var) = afp( uusi );
			NLset = 1;
			NLtype = tag->Number;
			break;

		case VAR_FREQ:
			result = ParseNote( uusi, (r_float *)var,
							(signed char *)( (r_float *)var + 1 ));
			if( result != 0 )
				err = "Illegal frequency/note.";
			break;

		default:
			err = "Bug: Unknown Tag->Type (parse2).";
			break;
	}
	free( uusi );
	return( err );
}

/*		char *InterpTagNumber( struct Tag *tag, char *beg, void *var );
 *
 *	Lukee beg:n osoittaman tekstin VAR_CUSTOM-*tag:n arvona
 *	ja sijoittaa sen *var:n. Erottaa tagit numeron perusteella elikä
 *	tämä funktio tulkkaa arvot niille tageille, jotka vaativat yksilöllistä
 *	tulkintaa. Esim. nuotin nimen muunnos nuotin numeroksi
 *	Palauttaa virheilmoituksen InterpTagType():lle, joka palauttaa sen eteenpäin.
 */

static char *InterpTagNumber( struct Tag *tag, char *beg, void *var )
{
	char *err = NULL;

#ifdef DEBUG
	DEBUGMSG( 11, "parse2: interpret tag by its id number" )
#endif

	if( tag == NULL || beg == NULL || var == NULL )
		return( "bug" );

	switch( tag->Number )
	{
		case T_WAVEFORM:
			if( CmpS( beg, WFNAME_SQUARE ))
				*((unsigned char *)var) = WAVE_SQUARE;
			else if( CmpS( beg, WFNAME_SAWTOOTH ))
				*((unsigned char *)var) = WAVE_SAWTOOTH;
			else if( CmpSbeg( beg, WFNAME_INPUT ))
				*((unsigned char *)var) = WAVE_INPUT;
			else if( CmpSbeg( beg, WFNAME_ONE_SHOT ))
				*((unsigned char *)var) = WAVE_ONE_SHOT;
			else if( CmpSbeg( beg, WFNAME_LOOPED ))
				*((unsigned char *)var) = WAVE_LOOPED;
			else
				err = "Unknown waveform type.";
			break;

		case T_OUT_FILEF:
		case T_WAVE_FILEF:
			if( CmpSbeg( FFNAME_BYTE, beg ))
				*((unsigned char *)var) = FILE_BYTE;
			else if( CmpSbeg( FFNAME_WORD, beg ))
				*((unsigned char *)var) = FILE_WORD;
			else if( CmpSbeg( FFNAME_FLOAT, beg ))
				*((unsigned char *)var) = FILE_FLOAT;
			else
				err = "Unknown file format.";
			break;

		case T_N_MODE:
			if( CmpSbeg( MNAME_NORMAL, beg ))
				*((unsigned char *)var) = MODE_NORMAL;
			else if( CmpSbeg( MNAME_REST, beg ))
				*((unsigned char *)var) = MODE_REST;
			else if( CmpSbeg( MNAME_TIE, beg ))
				*((unsigned char *)var) = MODE_TIE;
			else
				err = "Unknown note mode.";
			break;

		case T_N_LENGTH:
			if( CmpSbeg( LNAME_NORMAL, beg ))
				*((unsigned char *)var) = LEN_NORMAL;
			else if( CmpSbeg( LNAME_LEGATO, beg ))
				*((unsigned char *)var) = LEN_LEGATO;
			else if( CmpSbeg( LNAME_STACCATO, beg ))
				*((unsigned char *)var) = LEN_STACCATO;
			else if( CmpSbeg( LNAME_PORTATO, beg ))
				*((unsigned char *)var) = LEN_PORTATO;
			else
			{
				int tempo;
				if( ReadNum( beg, &tempo, 0, 100 ))
					err = "Illegal note length.";
				*((unsigned char *)var) = tempo;
			}
			break;

		case T_SUSTAIN:
			Comma2Period( beg );
			*((r_float *)var) = afp( beg );
			Data.SustainF = 1;
			break;

		case T_FIL_TYPE:
#if 0
			if( CmpS( FILTNAME_LPF, beg ))
				*((unsigned char *)var) = FILT_LPF;
			else if( CmpS( FILTNAME_HPF, beg ))
				*((unsigned char *)var) = FILT_HPF;
			else if( CmpS( FILTNAME_BPF, beg ))
				*((unsigned char *)var) = FILT_BPF;
			else if( CmpS( FILTNAME_BRF, beg ))
				*((unsigned char *)var) = FILT_BRF;
			else
				err = "Unknown filter type.";
#else
			if( CmpS( FILTNAME_HAMRE, beg ))
				*((unsigned char *)var) = FILT_HAMRE;
			else if( CmpS( FILTNAME_2POLE, beg ))
				*((unsigned char *)var) = FILT_2POLE;
			else
				err = "Unknown filter type.";
			break;
#endif

		default:
			err = "Bug: Unknown tag number (parse2).";
			break;
	}
	return( err );
}

/*		int ParseNote( char *asc, r_float *freq, signed char *notenum );
 *
 *	Tulkitsee asc:n osoittaman merkkijonon nuotin nimeksi, numeroksi tai
 *	absoluuttiseksi taajuudeksi. Mikäli asc osoittaa nuotin nimeen tai '#'-merkin
 *	edeltämään MIDI-numeroon [0, 127], asettaa nuotin numeron *notenum:n.
 *	Jos taas asc osoittaa taajuusmerkkijonoon (numeroita), asettaa taajuuden
 *	*freq:n.
 *	Alue on C-1...G9; keski-c on C4 eli numerona 60
 *	(täysimittaisen pianon koskettimisto on A0...C8 eli 21...108)
 *		hyväksytyt nuottien nimet:
 *	c c# d d# e f f# g g# a a# b
 *	c db d eb e f gb g ab a bb b
 *	Hyväksyy myös merkit '-', 'r' ja 't' tauoksi, jolloin asettaa taajuudeksi 0.
 *	Mikäli teksti esittää absoluuttista taajuutta, asettaa *notenum:n -1.
 *	Palauttaa -1:n, jos teksti ei ole hyväksyttävä.
 */

static int ParseNote( char *asc, r_float *freq, signed char *notenum )
{
	int num = 0;	/* 'c-1'=0 */

	if( asc == NULL || freq == NULL || notenum == NULL )
		return( -1 );

	switch( *asc )
	{
		case 0:
			return( -1 );
		case 'c':
		case 'C':
			break;
		case 'd':
		case 'D':
			num = 2;
			break;
		case 'e':
		case 'E':
			num = 4;
			break;
		case 'f':
		case 'F':
			num = 5;
			break;
		case 'g':
		case 'G':
			num = 7;
			break;
		case 'a':
		case 'A':
			num = 9;
			break;
		case 'b':
		case 'B':
			num = 11;
			break;
		case '-':	/* tauko */
		case 'r':
		case 'R':
		case 't':
		case 'T':
			*freq = FP0;
			*notenum = -1;
			return( 0 );
		case '#':	/* MIDI-nuotin numero */
			if( !ReadNum( asc + 1, &num, 0, 127 ))
			{
				*notenum = num;
				return( 0 );
			}
			return( -1 );
		default:	/* ei mikään edeltävistä; siispä se on taajuus */
			Comma2Period( asc );
			*freq = afp( asc );
			*notenum = -1;
			return( 0 );
	}
	asc++;
	if( *asc == '#' )
	{
		num++;
		asc++;
	}
	else if( *asc == 'b' && num )
	{
		num--;
		asc++;
	}
	if( *asc == '-' && *( asc + 1 ) != '1' )
		return( -1 );
	else if( isdigit( *asc ))
		num += 12 * ( *asc - '0' + 1 );
	else
		return( -1 );
	asc++;
	if( *asc != 0 )
		return( -1 );
	*notenum = num;
	return( 0 );
}

#if 0
/* hajottaa stringin [v]a.bx -> pri = a, sec = b, post = x */
static BOOL ParseVersion( char *ver, int *pri, int *sec, char **post )
{
	int i = 0;
	if( *ver == 'v' || *ver == 'V' )
		i++;
	if( !ver[ i ] )
		return( 0 );
		/* ... vaiheessa; lienekkö edes tarpeellinen */
}
#endif

/*		char *SetTextVar( char **var, char *text );
 *
 *	Asettaa *var:n text-merkkijonon. Varaa tarvittavan muistin ja kopioi
 *	text-merkkijonon tähän muistiin. Vapauttaa myös tarvittaessa edelliselle
 *	merkkijonolle varatun muistin. Mikäli text==NULL, niin vapauttaa
 *	muistin, mikäli sitä on varattu, ja asettaa *var=NULL.
 *	Palauttaa virheilmoituksen.
 */

char *SetTextVar( char **var, char *text )
{
	char *mem, *tend = text;

	if( *var )
		free( *var );
	if( text == NULL )
	{
		*var = NULL;
		return( NULL );
	}

	while( *tend )
		tend++;
	mem = (char *)malloc( (unsigned long)tend - (unsigned long)text + 1L );
	if( mem == NULL )
		return( "Out of memory." );
	strcpy( mem, text );
/* varmistetaan trailing null */
	*((char *)( (unsigned long)mem + (unsigned long)tend -
				(unsigned long)text )) = 0;
	*var = mem;
	return( NULL );
}

		/* ---		    Loppu		--- */
