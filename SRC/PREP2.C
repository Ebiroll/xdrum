/*
 *	prep.c		valmistelee sorsasta luetut arvot tb:lle
 *				3MU © Jarno Seppänen, 1996
 */

#include "3MU.h"
#include "r_ffp.h"

#include "debug.h"

/*	decay  = (65535-Decay)/65535
	release= (65535-Release)/65535
	envmod = EnvMod/65535
	attack = 1.1
	vol    = Volume/100
	reso   = (65536-Resonance)/65536
	cutoff = Cutoff/65535
	vcofreq= ((Detune/100)+Note+Transpose)/12
 */
		/* ---	   Funktioiden prototyypit	--- */

long Prep( struct Settings *set, struct Note *not );
static void PrepSettings( struct Settings *s );
static void PrepNote( struct Note *n );

		/* ---		   Funktiot		--- */

long Prep( struct Settings *set, struct Note *not )
{
	if( !set || !not )
		return( -1 );
	PrepSettings( set );
	for( int i = 0; i < set->Length; i++ )
		PrepNote( &not[ i ]);
	return( 0 );
}

static void PrepSettings( struct Settings *s )
{
	DEBUGMSG( 5, "prep: prepare settings" )

#ifdef DEBUG
		if( s->OutFrequency == FP0 )
		{
			DEBUGMSG( 0, "prep: almost divided by zero" );
			return;
		}
#endif
	s->Attack		= r_mul( r_div( s->Attack, FP1E2 ), afp( "1.1" ));
	s->Decay		= r_div( r_sub( FP1E4, s->Decay ), s->OutFrequency );
	s->Sustain		= r_sub( FP1, r_div( s->Sustain, FP1E2 ));
	s->Release		= r_div( r_sub( FP1E4, s->Release ), s->OutFrequency );
	s->EnvModAmp	= r_div( s->EnvModAmp, FP1E2 );
	s->EnvModVCF	= r_div( s->EnvModVCF, FP1E2 );
	s->Resonance	= r_sub( FP1, r_div( s->Resonance, FP1E2 ));
	s->Cutoff		= r_div( s->Cutoff, FP1E2 );
	s->Detune		= r_add( r_div( s->Detune, FP1E2 ), r_itof( s->Transpose ));

	s->ADSRFactor	= r_div( s->ADSRFactor, FP1E2 );
	s->VCFmodFactor	= r_div( s->VCFmodFactor, FP1E2 );
	s->AmpModFactor	= r_div( s->AmpModFactor, FP1E2 );
	s->ResoFactor	= r_div( s->ResoFactor, FP1E2 );
	s->CutoffFactor	= r_div( s->CutoffFactor, FP1E2 );
	s->VolumeFactor	= r_div( s->VolumeFactor, FP1E2 );
}

static void PrepNote( struct Note *n )
{
	DEBUGMSG( 5, "prep: prep note" )

	n->Detune		= r_div( n->Detune, FP1E2 );
	n->Attack		= r_div( n->Attack, FP1E2 );
	n->Decay		= r_sub( FP1, r_div( n->Decay, FP1E2 ));
	n->Sustain		= r_sub( FP1, r_div( n->Sustain, FP1E2 ));
	n->Release		= r_sub( FP1, r_div( n->Release, FP1E2 ));
	n->Cutoff		= r_div( n->Cutoff, FP1E2 );
	n->Resonance	= r_sub( FP1, r_div( n->Resonance, FP1E2 ));
	n->Volume		= r_div( n->Volume, FP1E2 );
	n->EnvModVCF	= r_div( n->EnvModVCF, FP1E2 );
	n->EnvModAmp	= r_div( n->EnvModAmp, FP1E2 );
}

		/* ---		    Loppu		--- */
