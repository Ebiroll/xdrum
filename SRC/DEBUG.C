/*
 *	debug.c			debugging routines
 *
 *	Copyright © 1996 Jarno Seppänen; see 3MU.c for licensing info.
 */

#include "debug.h"
#ifdef DEBUG

#include <stdio.h>
#include "3MU.h"

#include "r_ffp/r_ffp.h"

/* printable-100-times-float */
#define prf(f) ((int)(r_round(r_mul((f),FP1E2))))
/* printable-string */
#define prs(s) (((s)!=NULL)?(s):"<NULL>")

void DumpAllData( struct Settings *a, struct Note *s );
void DumpSettings( struct Settings *a );
void DumpNote( struct Note *n );

/*		void DumpAllData( struct Settings *a, struct Note *s )
 *
 *	tulostaa a:n ja kaikkien s:ien arvot.
 */

void DumpAllData( struct Settings *a, struct Note *s )
{
	int i;
	fputs( "\t--\tALL DATA (floatit 100-kert.):\n\tSETTINGS:\n", stderr );
	DumpSettings( a );

	fputs( "\tNOTES:\n", stderr );
	for( i = 0; i < a->Length; i++ )
	{
		fprintf( stderr, "\tnumero %d\n", i );
		DumpNote( &s[ i ]);
	}
}

/*		void DumpSettings( struct Settings *a )
 *
 *	Tulostaa a:n arvot.
 */

void DumpSettings( struct Settings *a )
{
	fprintf( stderr,

	"ID:  \"%s\"\n"
	"Out: (name=\"%s\" form=%d sfreq=%d)\n"
	"EG:  A=%d D=%d S=%d R=%d fact=%d\n"
	"VCO: wave=%d HPF=%d (name=\"%s\" form=%d sfreq=%d freq=%d)\n"
	"     len=%d pratio=%d (envmod=%d fact=%d)\n"
	"     detune=%d transpose=%d tuning=%d portatime=%d\n"
	"VCF: (freq=%d fact=%d) (reso=%d fact=%d) (envmod=%d fact=%d)\n"
	"     poles=%d type=%d amount=%d\n"
	"VCA: (envmod=%d fact=%d) (gain=%d fact=%d)\n"
	"Seq: length=%d rep=%d notelen=%d\n",

	prs( a->IDstring ),
	prs( a->OutFilename ), (int)a->OutFileformat, prf( a->OutSFrequency ),

	prf( a->Attack ), prf( a->Decay ), prf( a->Sustain ),
	prf( a->Release ), prf( a->ADSRFactor ),

	(int)a->Waveform, (int)a->HPF,
	prs( a->WaveFilename ), (int)a->WaveFileformat, prf( a->WaveSFrequency ),
	prf( a->WaveFrequency ), a->WaveLength, prf( a->PulseRatio ),
	prf( a->EnvModPitch ), prf( a->PitchModFactor ),
	prf( a->Detune ), (int)a->Transpose, prf( a->Tuning ),
	prf( a->PortaTime ),

	prf( a->Cutoff ), prf( a->CutoffFactor ),
	prf( a->Resonance ), prf( a->ResoFactor ),
	prf( a->EnvModVCF ), prf( a->VCFmodFactor ),
	(int)a->VCFpoles, (int)a->VCFtype, prf( a->VCFamount ),

 	prf( a->EnvModAmp ), prf( a->AmpModFactor ), 
	prf( a->Gain ), prf( a->VolumeFactor ),
	a->Length, a->Repeats, prf( a->NoteLen )

	);
}

/*		void DumpNote( struct Note *n )
 *
 *	Tulostaa n:n arvot.
 */

void DumpNote( struct Note *n )
{
	fprintf( stderr,

	"num=%d mode=%d accent=%d porta=%d freq=%d length=%d\n"
	"detune=%d /`-. A=%d D=%d S=%d R=%d\n"
	"cutoff=%d reso=%d vol=%d envmod:(VCF=%d VCA=%d VCO=%d)\n--\n",

	(int)n->Note, (int)n->Mode, (int)n->Accent, (int)n->Porta,
	prf( n->Frequency ), (int)n->Length, prf( n->Detune ),
	prf( n->Attack ), prf( n->Decay ), prf( n->Sustain ),
	prf( n->Release ), prf( n->Cutoff ), prf( n->Resonance ),
	prf( n->Volume ), prf( n->EnvModVCF ), prf( n->EnvModAmp ),
	prf( n->EnvModPitch )

	);
}
#endif	/* DEBUG */
