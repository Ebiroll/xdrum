/*
 *	prep.c			valmistelee sorsasta luetut, k‰ytt‰j‰n antamat arvot tb:lle
 *
 *	Copyright © 1996 Jarno Sepp‰nen; see 3MU.c for licensing info.
 */

#include <stdio.h>

#include "3MU.h"
#include "console.h"
#include "convaud.h"
#include "debug.h"

#include "r_ffp/r_ffp.h"
#include "misc_pro.h"
#include "scrio_pr.h"

#define	FP12	r_itof( 12 )
#define	FP69	r_itof( 69 )

		/* ---	   Funktioiden prototyypit	--- */

long Prep( struct Settings *set, struct Note *not );
static long PrepSettings( struct Settings *s );
static long PrepNote( struct Note *n, struct Settings *s );
static r_float NoteFreq( r_float note, r_float tuning );

		/* ---		   Funktiot		--- */

/*		long Prep( struct Settings *set, struct Note *not );
 *
 *	Valmistelee parametrit tb-konetta varten. set ja not itsest‰‰nselvi‰.
 *	Palauttaa virhekoodin.
 */

long Prep( struct Settings *set, struct Note *not )
{
	int i;

	if( set == NULL || not == NULL )
		return( -1 );

	if( PrepSettings( set ))
		return( -1 );

	for( i = 0; i < set->Length; i++ )
		if( PrepNote( &not[ i ], set ))
			return( -1 );

	return( 0 );
}

/*		long PrepSettings( struct Settings *s );
 *
 *	Valmistelee globaalit parametrit (Settings).
 */

static long PrepSettings( struct Settings *s )
{
#ifdef DEBUG
	DEBUGMSG( 5, "prep: prepare settings" )
#endif

	/* k‰ytt‰j‰yst‰v‰llisyys */

	s->ADSRFactor	= r_div( s->ADSRFactor, FP1E2 );
	s->VCFmodFactor	= r_div( s->VCFmodFactor, FP1E2 );
	s->AmpModFactor	= r_div( s->AmpModFactor, FP1E2 );
	s->PitchModFactor=r_div( s->PitchModFactor, FP1E2 );
	s->ResoFactor	= r_div( s->ResoFactor, FP1E2 );
	s->CutoffFactor	= r_div( s->CutoffFactor, FP1E2 );
	s->VolumeFactor	= r_div( s->VolumeFactor, FP1E2 );

	s->PulseRatio	= r_div( s->PulseRatio, FP1E2 );
	s->VCFamount	= r_div( s->VCFamount, FP1E2 );
	s->Detune		= r_div( s->Detune, FP1E2 );
	s->PortaTime	= r_div( s->PortaTime, FP1E2 );

	/* vars. arvoja */

	s->Detune		= r_add( s->Detune, r_itof( s->Transpose ));

	if( s->OutSFNote >= 0 )
		s->OutSFrequency = NoteFreq( r_itof( s->OutSFNote ), s->Tuning );
	if( s->WaveSFNote >= 0 )
		s->WaveSFrequency = NoteFreq( r_itof( s->WaveSFNote ), s->Tuning );
	if( s->WaveFNote >= 0 )
		s->WaveFrequency = NoteFreq( r_itof( s->WaveFNote ), s->Tuning );
	if( s->CutoffNote >= 0 )
		s->Cutoff = NoteFreq( r_itof( s->CutoffNote ), s->Tuning );

	if( s->OutSFrequency == FP0 )
	{
		fputs( "Zero output sample frequency.\n", stderr );
		return( -1 );
	}
	if( s->Length == 0 )
	{
		fputs( "Zero sequence length.\n", stderr );
		return( -1 );
	}
	if( s->Waveform == WAVE_INPUT )
	{
		r_float nlen;

		if( s->WaveSFrequency == FP0 )
			s->WaveSFrequency = s->OutSFrequency;

		/*	lasketaan yhden nuotin pituus siten, ett‰ sekvenssin pituus
		 *	osuu yksiin luettavan tiedoston pituuden kanssa
		 */
		nlen = r_div( s->OutSFrequency, s->WaveSFrequency );
		nlen = r_mul( r_itof( s->WaveLength ), nlen );
		nlen = r_div( nlen, r_itof( s->Length ));

		if( s->NoteLen == FP0 || !Verbose )
			s->NoteLen = nlen;
		else if( s->NoteLen != nlen )
		{
			fputs(	"Input file duration is different from the sequence duration\n"
					"requested for. Should I adapt to the input file duration and\n"
					"thereby override source file setting? Y" bwd( "1" ), stdout );
			if( getchar() != 'n' )
				s->NoteLen = nlen;
			getchar();		/* line feed */
		}
	}
	if( s->NoteLen == FP0 )
	{
		fputs( "Zero note sample length.\n", stderr );
		return( -1 );
	}

	s->PortaTime = r_mul( s->PortaTime, s->NoteLen );

	if( s->PortaTime == FP0 )
	{
//		fputs( "Zero portamento length.\n", stderr );
		return( -1 );
	}
	return( 0 );
}

/*		long PrepNote( struct Note *n, struct Settings *s );
 *
 *	Valmistelee yhden nuotin arvot.
 */

static long PrepNote( struct Note *n, struct Settings *s )
{
#ifdef DEBUG
	DEBUGMSG( 5, "prep: prep note" )
#endif

	/* factorit */

	n->Attack		= r_add( s->Attack,    r_mul( n->Attack,    s->ADSRFactor ));
	n->Decay		= r_add( s->Decay,     r_mul( n->Decay,     s->ADSRFactor ));
	n->Sustain		= r_add( s->Sustain,   r_mul( n->Sustain,   s->ADSRFactor ));
	n->Release		= r_add( s->Release,   r_mul( n->Release,   s->ADSRFactor ));
	n->Cutoff		= r_add( s->Cutoff,    r_mul( n->Cutoff,    s->CutoffFactor));
	n->Resonance	= r_add( s->Resonance, r_mul( n->Resonance, s->ResoFactor ));
	n->EnvModVCF	= r_add( s->EnvModVCF, r_mul( n->EnvModVCF, s->VCFmodFactor));
	n->EnvModAmp	= r_add( s->EnvModAmp, r_mul( n->EnvModAmp, s->AmpModFactor));
	n->EnvModPitch	= r_add( s->EnvModPitch,
						r_mul( n->EnvModPitch, s->PitchModFactor ));

	/* k‰ytt‰j‰yst‰v‰llisyyskertoimet */

	n->Attack		= r_div( n->Attack, FP1E3 );
	n->Decay		= r_div( n->Decay, FP1E3 );
	n->Sustain		= r_div( n->Sustain, FP1E2 );
	n->Release		= r_div( n->Release, FP1E3 );
	n->EnvModVCF	= r_div( n->EnvModVCF, FP1E2 );
	n->EnvModAmp	= r_div( n->EnvModAmp, FP1E2 );
	n->EnvModPitch	= r_div( n->EnvModPitch, FP1E2 );
	n->Volume		= r_div( n->Volume, FP1E2 );
	n->Detune		= r_div( n->Detune, FP1E2 );

	if( n->Attack == FP0 || n->Decay == FP0 || n->Release == FP0 )
	{
		fputs( "Zero attack, decay or release time.\n", stderr );
		return( -1 );
	}
	if( n->Resonance == FP0 )
	{
		fputs( "Zero resonance.\n", stderr );
		return( -1 );
	}

	/* s‰‰det‰‰n varsinaiset arvot muuttujille */

	n->Detune		= r_add( n->Detune, s->Detune );
	if( n->Note2 >= 0 )
		n->Frequency = NoteFreq( r_add( r_itof( n->Note2 ), n->Detune ),
									s->Tuning );
	n->Frequency	= r_div( n->Frequency, s->OutSFrequency );
	n->Attack		= r_pow( r_div( FP1, ATTACKC ),
						r_div( FP1, r_mul( s->OutSFrequency, n->Attack )));
	n->Decay		= r_pow( FP0_5,
						r_div( FP1, r_mul( s->OutSFrequency, n->Decay )));
	n->Release		= r_pow( FP0_5,
						r_div( FP1, r_mul( s->OutSFrequency, n->Release )));
	if( n->CutoffNote >= 0 )
		n->Cutoff	= NoteFreq( r_itof( n->CutoffNote ), s->Tuning );
	n->Cutoff		= r_div( n->Cutoff, s->OutSFrequency );
	n->Resonance	= r_div( FP1, n->Resonance );
	n->Volume		= r_add( r_sub( FP1, s->VolumeFactor ),
						r_mul( n->Volume, s->VolumeFactor ));
	n->Volume		= r_mul( dB2ffp( s->Gain ), n->Volume );

	return( 0 );
}

/*		r_float NoteFreq( r_float note, r_float tuning )
 *
 *	Muuntaa MIDI-nuotin taajuudeksi.
 *	note on midi-nuotin numero ynn‰ pitch sun muut bendit ja
 *	tuning on 1-viivaisen a:n taajuus hertsein‰ (440 yleens‰).
 *	Palauttaa nuotin taajuuden hertsein‰.
 */

static r_float NoteFreq( r_float note, r_float tuning )
{
	note = r_div( r_sub( note, FP69 ), FP12 );
	return( r_mul( r_pow( FP2, note ), tuning ));
}

		/* ---		    Loppu		--- */
