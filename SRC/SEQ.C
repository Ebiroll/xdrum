/*
 *	seq.c			sound production engine v1.0js
 *
 *	Copyright © 1995, 1996 Jarno Sepp‰nen; see 3MU.c for licensing info.
 *
 *	initially by Lars Hamre <lars@scala.no>, 1995
 *	edited by Jarno Sepp‰nen <jarno.seppanen@cc.tut.fi>
 *
 */

#include "3MU.h"
#include "convaud.h"
#include "debug.h"

#include "r_ffp/r_ffp.h"
#include "eg_proto.h"
#include "io_proto.h"
#include "misc_pro.h"
#include "vco_prot.h"
#include "vcf_prot.h"
#include "vca_prot.h"

/* normaali taajuuden muutoksen kesto sekunteina */
#ifdef _DCC
#define NOPORTATIME afp( "0.001" )
#else
#define NOPORTATIME (0.001)
#endif

/* OLAS Dont know if this is necessary?? */
extern r_float Limit( r_float sample );


		/* ---	   Funktioiden prototyypit	--- */

long TB( struct Settings *dat, struct Note *not );

unsigned long TBlen( struct Settings *dat, struct Note *not );
unsigned long TBsize( struct Settings *dat, struct Note *not );
r_float TBdur( struct Settings *dat, struct Note *not );
unsigned short TBnoteDur( struct Settings *dat, struct Note *not, long inlen );
r_float PeakLevel( void );
r_float OptimalGain( r_float Gain );

static long TBinit(  struct Settings *dat, struct Note *not );
static long TBsound( r_float *sample, struct Note *nyt, r_float relnotepos,
						BOOL trigger, char nextmode );
static long TBquit( void );

		/* ---		  Muuttujat		--- */

char *EngineName		= ENGNAME;
char *EngineVersion	= ENGVERS;

static r_float AbsMax	= FP0;	/* itseisarvojen maksimi */
static r_float input_delta;

/* data erikseen */
static struct Note *	sequ;
static struct Settings *data;




		/* ---		   Funktiot		--- */

/*		long OlofsTB( struct Settings *dat, struct Note *not );
 *
 *      Works as TB, but puts deta in result!!!
 */

long OlofsTB( struct Settings *dat, struct Note *not,short *Result )
{
	long err = 0;

	r_float sample,fout;
	r_float time_diff = FP0, time_dec;		/* 0 <= time_diff < 1	*/
	unsigned long notetime, time = 0;		/* 0 <= time < NoteLen	*/
	BOOL longnote = 0, trigger;
	int notenum = 0;
	struct Note *nyt;
	r_float absout;
	short out;

#ifdef DEBUG
	DEBUGMSG( 2, "tb" )
#endif
	if( dat == NULL || not == NULL )
		return( -2 );

	if( TBinit( dat, not ))
		return( -1 );

	notetime = r_trunc( data->NoteLen );
	time_dec = r_sub( data->NoteLen, r_itof( notetime ));

	while( data->Repeats )
	{
		trigger = 0;
		/* Nuotin alussa asetetaan nuottiosoitin */
		if( time == 0 )
		{
			nyt = &sequ[ notenum ];
#ifdef DEBUG
			IFDEBUG( 4 )
			{
				fputs( "\nRead note:\n", stderr );
				DumpNote( nyt );
			}
#endif
			if( nyt->Mode == MODE_NORMAL )
				trigger = 1;
		}

		err = TBsound( &sample, nyt, r_div( r_itof( time ), data->NoteLen ),
					   trigger, sequ[( notenum + 1 ) % data->Length ].Mode );
		if( err )
			break;

		/* etsit‰‰n n‰ytteiden itseisarvojen maksimi */
		absout = r_fabs( sample );
		if( r_cmpable( absout ) > r_cmpable( AbsMax ))
			AbsMax = absout;

		/* UpdPercent( sizeof( short ), sample ); */
		// sample=sample; ///(float) 6000.0;
		fout = Limit( sample );
//		out = ffp2int( fout,FP32768 );
		out = fout*32768;

		*Result=out;
			//(out^0x8000);
		Result++;	

#ifdef OLLEPOLLE

		if( WriteSample( outfile, sample, data->OutFileformat ))
		{
			fputs( "Couldn't write to output file.\n", stderr );
			err = -1;
			break;
		}
#endif
		time += 1;	/* Increase time in the middle of a note */

		/* At the end of a note, start another one */
		if( time >= notetime )
		{
			time = 0;
			time_diff = r_add( time_diff, time_dec );
			if( r_cmpable( time_diff ) >= r_cmpable( FP1 ))
			{
				time_diff = r_sub( time_diff, FP1 );
				notetime++;
				longnote = 1;
			} else
				if( longnote )
				{
					notetime--;
					longnote = 0;
				}
			notenum++;
			if( notenum >= data->Length )
			{
				data->Repeats--;
				notenum = 0;
			}
		}
	}
	if( TBquit())
		return( -1 );
	return( err );
}



/*		long TB( struct Settings *dat, struct Note *not );
 *
 *	Emulaattorirutiinin p‰‰funktio. *dat:ssa on globaalit muuttujat,
 *	yleiset asetukset ja *not:ssa on sekvenssi nuotteja (struct Note).
 *	N‰iden mukaisesti syntetisoidaan ‰‰nin‰yte, joka yksitt‰inen n‰yte
 *	kerrallaan syˆtet‰‰n STDOUT:iin tai io.c:lle tiedostoon tallennettavaksi.
 *	Palauttaa virhekoodin.
 */

long TB( struct Settings *dat, struct Note *not )
{
	long err = 0;

	r_float sample;
	r_float time_diff = FP0, time_dec;		/* 0 <= time_diff < 1	*/
	unsigned long notetime, time = 0;		/* 0 <= time < NoteLen	*/
	BOOL longnote = 0, trigger;
	int notenum = 0;
	struct Note *nyt;
	r_float absout;

#ifdef DEBUG
	DEBUGMSG( 2, "tb" )
#endif
	if( dat == NULL || not == NULL )
		return( -2 );

	if( TBinit( dat, not ))
		return( -1 );

	notetime = r_trunc( data->NoteLen );
	time_dec = r_sub( data->NoteLen, r_itof( notetime ));

	while( data->Repeats )
	{
		trigger = 0;
		/* Nuotin alussa asetetaan nuottiosoitin */
		if( time == 0 )
		{
			nyt = &sequ[ notenum ];
#ifdef DEBUG
			IFDEBUG( 4 )
			{
				fputs( "\nRead note:\n", stderr );
				DumpNote( nyt );
			}
#endif
			if( nyt->Mode == MODE_NORMAL )
				trigger = 1;
		}

		err = TBsound( &sample, nyt, r_div( r_itof( time ), data->NoteLen ),
					   trigger, sequ[( notenum + 1 ) % data->Length ].Mode );
		if( err )
			break;

		/* etsit‰‰n n‰ytteiden itseisarvojen maksimi */
		absout = r_fabs( sample );
		if( r_cmpable( absout ) > r_cmpable( AbsMax ))
			AbsMax = absout;

		if( WriteSample( outfile, sample, data->OutFileformat ))
		{
			fputs( "Couldn't write to output file.\n", stderr );
			err = -1;
			break;
		}

		time += 1;	/* Increase time in the middle of a note */

		/* At the end of a note, start another one */
		if( time >= notetime )
		{
			time = 0;
			time_diff = r_add( time_diff, time_dec );
			if( r_cmpable( time_diff ) >= r_cmpable( FP1 ))
			{
				time_diff = r_sub( time_diff, FP1 );
				notetime++;
				longnote = 1;
			} else
				if( longnote )
				{
					notetime--;
					longnote = 0;
				}
			notenum++;
			if( notenum >= data->Length )
			{
				data->Repeats--;
				notenum = 0;
			}
		}
	}
	if( TBquit())
		return( -1 );
	return( err );
}

/*		unsigned long TBlen( struct Settings *dat, struct Note *not );
 *
 *	Palauttaa *dat- ja *not-tietojen perusteella syntetisoidun n‰ytteen
 *	pituuden n‰yttein‰. Palauttaa nollan, jos kutsutaan TB():n j‰lkeen.
 */

unsigned long TBlen( struct Settings *dat, struct Note *not )
{
	if( dat == NULL || not == NULL )
		return( 0 );
	return( dat->Length * dat->Repeats * r_round( dat->NoteLen ));
}

/*		unsigned long TBsize( struct Settings *dat, struct Note *not );
 *
 *	Laskee *dat- ja *not-tietojen perusteella syntetisoidun n‰ytteen
 *	pituuden tavuina ja palauttaa sen.
 *	Palauttaa nollan, jos kutsutaan TB():n j‰lkeen.
 */

unsigned long TBsize( struct Settings *dat, struct Note *not )
{
	unsigned long temp;
	if( dat == NULL || not == NULL )
		return( 0 );
	temp = TBlen( dat, not );
	switch( dat->OutFileformat )
	{
		case FILE_BYTE:
			return( temp * sizeof( signed char ));
		case FILE_WORD:
			return( temp * sizeof( short ));
		case FILE_FLOAT:
			return( temp * sizeof( r_float ));
	}
	return( 0 );
}

/*		r_float TBdur( struct Settings *dat, struct Note *not );
 *
 *	Laskee *dat- ja *not-tietojen perusteella syntetisoidun n‰ytteen
 *	keston sekunteina ja palauttaa sen.
 *	Palauttaa nollan, jos kutsutaan TB():n j‰lkeen.
 */

r_float TBdur( struct Settings *dat, struct Note *not )
{
	if( dat == NULL || not == NULL )
		return( FP0 );
	if( dat->OutSFrequency == FP0 )
		return( FP0 );
	return( r_div( r_itof( TBsize( dat, not )), dat->OutSFrequency ));
}

/*		unsigned short TBnoteDur( struct Settings *dat, struct Note *not,
 *									long inlen );
 *
 *	Laskee *dat- ja *not-tietojen perusteella dat->NoteLen:lle sellaisen
 *	arvon, ett‰ sekvenssin pituus ja input-tiedoston pituus inlen osuvat yksiin
 *	Palauttaa nollan, jos kutsutaan TB():n j‰lkeen.
 */

unsigned short TBnoteDur( struct Settings *dat, struct Note *not, long inlen )
{
	r_float len;
	if( dat == NULL || not == NULL || !inlen )
		return( 0 );
	len = r_itof( inlen );
	switch( dat->WaveFileformat )
	{
		case FILE_BYTE:
			len = r_div( len, r_itof( sizeof( signed char )));
		case FILE_WORD:
			len = r_div( len, r_itof( sizeof( short )));
		case FILE_FLOAT:
			len = r_div( len, r_itof( sizeof( r_float )));
	}
	if( dat->Length == 0 || dat->Repeats == 0 )
		return( 0 );
	len = r_div( r_div( len, r_itof( dat->Length )), r_itof( dat->Repeats ));
	return( (unsigned short)r_round( len ));
}

/*		r_float PeakLevel( void )
 *
 *	Palauttaa tehdyn suurimman itseisarvon tehdyst‰ ‰‰nidatasta.
 */

r_float PeakLevel( void )
{
	return( AbsMax );
}

/*		r_float OptimalGain( r_float Gain );
 *
 *	Palauttaa optimaalisen kokonaislukugainin jo tehdyn samplen perusteella,
 *	tai ILLEGALGAIN:n, jos optimaalinen vahvistus ei ole saatavilla.
 */

r_float OptimalGain( r_float Gain )
{
	if( AbsMax == FP0 )
		return( ILLEGALGAIN );
	return( r_sub( ffp2dB( AbsMax ), Gain ));
}

static long TBinit(  struct Settings *dat, struct Note *not )
{
	r_float cycles;

	data = dat;
	sequ = not;

	if( data->OutSFrequency == FP0 )
	{
		fputs( "Zero sample freq (tb.c)\n", stderr );
		return( 1 );
	}
	if( data->Length == FP0 )
	{
		fputs( "Zero sequence len (tb.c)\n", stderr );
		return( 1 );
	}
	if( data->WaveFrequency == FP0 || data->WaveSFrequency == FP0 )
		cycles = FP1;
	else
	{
		if( data->WaveLength == FP0 )
		{
			fputs( "Zero wavetable len (tb.c)\n", stderr );
			return( 1 );
		}
		cycles = r_div( r_mul( data->WaveFrequency, r_itof( data->WaveLength )),
						data->WaveSFrequency );
	}
	if( data->Waveform == WAVE_INPUT )
	{
		if( data->NoteLen == FP0 || data->Length == 0 )
		{
			fputs( "tb: almost divided by zero\n", stderr );
			return( -1 );
		}
		input_delta = r_mul( data->NoteLen, r_itof( data->Length ));
		input_delta = r_div( r_itof( data->WaveLength ), input_delta );
	}
	else if( VCOinit( data->Waveform, data->HPF,
					data->WaveLength, data->WaveTable, cycles,
					data->PulseRatio, sequ[(data->Length)-1].Frequency,
					data->PortaTime,
					r_mul( NOPORTATIME, data->OutSFrequency )))
			return( -1 );
	if( VCFinit( data->VCFpoles, data->VCFtype ))
		return( -1 );
	return( 0 );
}

static long TBsound( r_float *sample, struct Note *note, r_float relnotepos,
						BOOL trig, char nextmode )
{
	static r_float input_phase = FP1, samp;
	r_float env = EG( note->Attack, note->Decay, ( data->SustainF ?
				dB2ffp( note->Sustain ) : FP0 ),
				note->Release, note->Accent, note->Length, trig,
				nextmode, relnotepos );
	if( data->Waveform == WAVE_INPUT )
	{
		while( r_cmpable( input_phase ) >= r_cmpable( FP1 ))
		{
			/* tod rankka `interpolointi' */
			if( ReadSample( infile, &samp, data->WaveFileformat ))
			{
				fputs( "Couldn't read from input file.\n", stderr );
				return( -1 );
			}
			input_phase = r_sub( input_phase, FP1 );
		}
		input_phase = r_add( input_phase, input_delta );
	}
	else
		samp = VCO( note->Frequency, r_mul( note->EnvModPitch, env ),
						note->Porta, trig );

#ifdef DEBUG
	IFDEBUG(5)
		if( WriteSample( outfile, samp, data->OutFileformat ))
		{
			fputs( "Couldn't write to output file.\n", stderr );
			return( -1 );
		}
#endif

	if( data->VCFamount && data->VCFpoles )
	{
		r_float fils = VCF( samp, r_mul( env, note->EnvModVCF ),
							note->Cutoff, note->Resonance );
		samp = r_add( r_mul( data->VCFamount, fils ),
						r_mul( r_sub( FP1, data->VCFamount ), samp ));
	}

#ifdef DEBUG
	IFDEBUG(5)
		if( WriteSample( outfile, r_div( samp, FP3 ), data->OutFileformat ))
		{
			fputs( "Couldn't write to output file.\n", stderr );
			return( -1 );
		}
#endif

	samp = VCA( samp, r_add( r_sub( FP1, note->EnvModAmp ),
					r_mul( env, note->EnvModAmp )), note->Volume );
	*sample = samp;
	return( 0 );
}

static long TBquit( void )
{
	if( VCFquit())
		return( -1 );
	return( 0 );
}

		/* ---		    Loppu		--- */
