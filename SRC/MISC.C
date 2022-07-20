/*
 *	misc.c			miscellaneous routines
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
#include "debug.h"

#include "r_ffp/r_ffp_pr.h"

#include "scrio_pr.h"
#include "seq_prot.h"

	/* käsitellään vain joka viides näyte */
#define	AVGSKIP			5
#ifdef _DCC
#define FPAVGINERTIA	afp( "100" )	/* 500/AVGSKIP */
#else
#define FPAVGINERTIA	(500.0/AVGSKIP)
#endif

		/* ---	   Funktioiden prototyypit	--- */

/* vain main():n käytössä */
void Check( struct Settings *set );

/* julkiset funktiot */
BOOL CmpSbeg( const char *first, const char *second );
BOOL CmpS( const char *first, const char *second );
int ReadNum( const char *text, int *dest, int min, int max );

r_float AvgLevel( r_float sample, r_float srate );

void StartTimer( void );
void StopTimer( void );
clock_t ReadTimer( void );
void ResetTimer( void );

void Comma2Period( char *a );

		/* ---		  Muuttujat		--- */

char *EnvironName		= ENVNAME;
char *EnvironVersion	= ENVVERS;
char *ReqEnvName = NULL, *ReqEngName = NULL;
char *ReqEnvVers = NULL, *ReqEngVers = NULL;
struct Settings	Data;
struct Note		Sequence[ MAXNOTES ];

FILE			*infile = NULL, *outfile = NULL;

static clock_t timer_total, timer_start;
static BOOL timer_running = 0;

		/* ---		   Funktiot		--- */

/*		void Check( struct Settings *set );
 *
 *	Vertaa 3MU-käännöksen ja synteesin lähdekoodissa pyydettyjen
 *	moottorin ja ympäristön nimiä ja versioita ja kertoo käyttäjälle
 *	mahdollisista eroavaisuuksista. set on globaalien tietojen tietueen osoite.
 */

/* sama engine, sama versio
 * sama engine, eri versio
 * sama engine, ei versiota
 * eri engine
 * eri engine, ei versiota
 * ei engineä
 * ei engineä, ei versiota
 */
void Check( struct Settings *set )
{
	BOOL ok = 1; /* menikö kaikki hyvin; ei tarvinnut varoitella? */

	if( !Verbose )
		return;

	if( ReqEngName != NULL )
	{
		if( strcmp( ReqEngName, EngineName ))
		{
			printf( ULON "Warning:" ULOFF
					"\nSource file is for engine `%s' while "
					"this engine is `%s'.\n", ReqEngName, EngineName );
			ok = 0;
		}
	} else
	{
#if 0
		puts( ULON "Warning:" ULOFF
				"\nNo engine specified in source file." );
#endif
	}

	if( ReqEngVers != NULL )
	{	if( strcmp( ReqEngVers, EngineVersion ))
		{
			if( ok )
			{
				puts( ULON "Warning:" ULOFF );
				ok = 0;
			}
			printf( "Source file is for version <%s> of engine `%s'.\n"
					"This engine version is <%s>.\n", ReqEngVers, EngineName,
					EngineVersion );
		}
	} else
	{
#if 0
		if( ok )
		{
			puts( ULON "Warning:" ULOFF );
			ok = 0;
		}
		puts( "No engine version specified in source file." );
#endif
	}

	if( ReqEnvName != NULL )
	{	if( strcmp( ReqEnvName, EnvironName ))
		{	if( ok )
			{
				puts( ULON "Warning:" ULOFF );
				ok = 0;
			}
			printf( "Source file is for environment `%s' while "
					"this environment is `%s'.\n", ReqEnvName, EnvironName );
		}
	} else
	{
#if 0
		if( ok )
		{
			puts( ULON "Warning:" ULOFF );
			ok = 0;
		}
		puts( "No environment specified in source file." );
#endif
	}

	if( ReqEnvVers != NULL )
	{
		if( strcmp( ReqEnvVers, EnvironVersion ))
		{
			if( ok )
			{
				puts( ULON "Warning:" ULOFF );
				ok = 0;
			}
			printf( "Source file is for version <%s> of environment `%s'.\n"
					"This environment version is <%s>.\n", ReqEnvVers, EnvironName,
					EnvironVersion );
		}
	} else
	{
#if 0
		if( ok )
		{
			puts( ULON "Warning:" ULOFF );
			ok = 0;
		}
		puts( "No environment version specified in source file." );
#endif
	}

	if( !ok )
		puts( "Output may be different from what intented." );
}

/*		BOOL CmpSbeg( const char *first, const char *second );
 *
 *	Vertailee first:stä ja second:stä alkavien merkkijonojen alkuja keskenään.
 *	Palauttaa 1:n jos lyhempi merkkijono on identtinen pidemmän mjonon alun
 *	kanssa olettaen, että isot ja pienet kirjaimet esittävät samaa asiaa.
 *	Muuten palauttaa 0:n.
 */

BOOL CmpSbeg( const char *first, const char *second )
{
	if( first == NULL || second == NULL )
		return( 0 );

	while( *first && *second )
	{
		if( tolower( *first ) != tolower( *second ))
			return( 0 );
		first++;
		second++;
	}
	return( 1 );
}

/*		BOOL CmpS( const char *first, const char *second );
 *
 *	Vertailee first:stä ja second:stä alkavia merkkijonoja keskenään.
 *	Palauttaa 1:n jos merkkijonot ovat identtiset olettaen, että
 *	isot ja pienet kirjaimet esittävät samaa asiaa. Muuten palauttaa 0:n.
 */

BOOL CmpS( const char *first, const char *second )
{
	if( first == NULL || second == NULL )
		return( 0 );

	while( *first && *second )
	{
		if( tolower( *first ) != tolower( *second ))
			return( 0 );
		first++;
		second++;
	}
	if( *first || *second )
		return( 0 );
	else
		return( 1 );
}

/*		int ReadNum( const char *text, int *dest, int min, int max )
 *
 *	Lukee numeron muuttujasta *text ja sijoittaa sen *dest:iin jos se
 *	a) on numero ja b) on välillä [min, max]. Palauttaa 0 jos kaikki tosiaan
 *	meni näin.
 */

int ReadNum( const char *text, int *dest, int min, int max )
{
	BOOL ok = 0;
	int num = 0;
	while( isdigit( *text ) && ( ok = (( num >= min ) && ( num <= max ))))
	{
		num *= 10;
		num += *text - '0';
		text++;
	}
	if( !ok || *text )
		return( -1 );
	*dest = num;
	return( 0 );
}

/*		r_float AvgLevel( r_float sample, r_float srate )
 *
 *	Palauttaa keskimääräisen signaalin tason [0, 1]
 */

r_float AvgLevel( r_float sample, r_float srate )
{
	static int skip = 0;
	static r_float avglevel = FP0;
	if( skip )
	{
		skip--;
		return( avglevel );
	}
	skip = AVGSKIP;
	sample = r_fabs( sample );
	avglevel = r_mul( avglevel, r_div( FPAVGINERTIA, srate ));
	if( r_cmpable( sample ) > r_cmpable( avglevel ))
		avglevel = sample;
	return( avglevel );
}

#if 0
static void Size2Str( char *dest, unsigned long size )
{
	dest[ 4 ] = 0;
	if( size >= 1024*1024*1024 )
	{
		dest[ 3 ] = 'G';
		size >>= 30;
	}
	else if( size >= 1024*1024 )
	{
		dest[ 3 ] = 'M';
		size >>= 20;
	}
	else if( size >= 1024 )
	{
		dest[ 3 ] = 'k';
		size >>= 10;
	}
	else
	{
		sprintf( dest, "%4d", (int)size );
		return;
	}
	sprintf( dest, "%3d", (int)size );
}
#endif

/*		void StartTimer( void )
 *
 *	Käynnistää ajastimen siitä mihin se oli jäänyt.
 */

void StartTimer( void )
{
	timer_running = 1;
	timer_start = clock();
}

/*		void StopTimer( void )
 *
 *	Pysäyttää ajastimen.
 */

void StopTimer( void )
{
	clock_t temp = clock();
	if( !timer_running )
		return;
	timer_running = 0;
	timer_total += temp - timer_start;
}

/*		clock_t ReadTimer( void )
 *
 *	Palauttaa ajastimen arvon, yksikkö clock():n yksikkö.
 */

clock_t ReadTimer( void )
{
	clock_t temp = clock();
	if( !timer_running )
		return( timer_total );
	return( timer_total + temp - timer_start );
}

/*		void ResetTimer( void )
 *
 *	Nollaa ajastimen.
 */

void ResetTimer( void )
{
	timer_total = 0;
}

/*		void Comma2Period( char *a );
 *
 *	Muuttaa a:n osoittamassa merkkijonossa olevan (ensimmäisen)
 *	pilkun ',' pisteeksi '.'.
 */

void Comma2Period( char *a )
{
	int i;
	for( i = 0; a[ i ]; i++ )
		if( a[ i ] == ',' )
		{
			a[ i ] = '.';
			break;
		}
}


extern long OlofsTB( struct Settings *dat, struct Note *not,short *Result );


#ifdef OLLEPOLLE
 struct Settings
{
	char *			IDstring;

	char *			OutFilename;	/* NULL indicates output to STDOUT		*/
	char *			WaveFilename;	/* NULL indicates input from STDIN		*/
	unsigned char	OutFileformat;
	unsigned char	WaveFileformat;	/* waveform file format					*/

/* Frequency and Note MUST follow each other, frequency first */

	r_float			OutSFrequency;	/* lähtönäytetaajuus					*/
	signed char		OutSFNote;		/* note number alternative for OutSFreq	*/
	char			pad1;

	r_float			WaveFrequency;	/* 0 if wavetable contains 1 cycle		*/
	signed char		WaveFNote;		/* note number alternative for WaveFreq	*/
	char			pad2;

	r_float			WaveSFrequency;	/* sample frequency						*/
	signed char		WaveSFNote;		/* note number alternative for WaveSFreq*/
	char			pad3;

	r_float			NoteLen;		/* yhden nuotin pituus näytteinä		*/

	unsigned char	Waveform;
	char			HPF;			/* high pass filter active				*/
	r_float *		WaveTable;		/* osoite								*/
	unsigned short	WaveLength;		/* pituus näytteinä						*/
	r_float			PulseRatio;		/* sisäisen kanttiaallon pulssisuhde	*/

	unsigned short	Length;			/* sekvenssin pituus nuotteina			*/
	unsigned short	Repeats;		/* kertausten lukumäärä					*/

	r_float			VCFamount;		/* wet/dry								*/
	unsigned char	VCFtype;		/* ks. alas								*/
	unsigned char	VCFpoles;
	r_float			Resonance;
	r_float			Cutoff;
	signed char		CutoffNote;		/* note number alternative for Cutoff	*/

	char			SustainF;		/* 1, jos halutaan sustainia, muuten 0	*/
	r_float			Attack;
	r_float			Decay;
	r_float			Sustain;
	r_float			Release;

	r_float			ADSRFactor;
	r_float			ResoFactor;
	r_float			CutoffFactor;
	r_float			VCFmodFactor;
	r_float			AmpModFactor;
	r_float			PitchModFactor;
	r_float			VolumeFactor;

	r_float			EnvModAmp;
	r_float			EnvModVCF;
	r_float			EnvModPitch;

	r_float			Gain;

	r_float			Detune;			 1/100 -puolisävelaskelta		
	r_float			Tuning;
	signed char		Transpose;
	char			pad6;
	r_float			PortaTime;

	unsigned long	Reserved[8];
};


 struct Note	/* sekvenssi koostuu näistä */
{
/* Frequency and Note MUST follow each other, frequency first */

	r_float			Frequency;	/* vastaa siis Notea, TB laskee				*/
	signed char		Note2;		/* nuotin MIDI-numero						*/

	char			Mode;		/* legato, staccato, tie, rest yms.			*/
	char			Accent;		/* 1=aksentti, 0=ei oo						*/
	char			Porta;		/* 1=portamento, 0=ei oo					*/

	r_float			Detune;
	r_float			Attack;
	r_float			Decay;
	r_float			Sustain;
	r_float			Release;
	r_float			Resonance;

	r_float			Cutoff;
	signed char		CutoffNote;	/* note number alternative for Cutoff		*/
	unsigned char	Length;		/* 0% = release aivan nuotin alussa, 100% = tie w/trig*/

	r_float			Volume;
	r_float			EnvModVCF;
	r_float			EnvModAmp;
	r_float			EnvModPitch;

	unsigned long	Reserved[5];
};


#endif


int SetMyDefaultData(struct Settings *TheData) {
Data.OutSFrequency=atof("4410000");	/*  lähtönäytetaajuus					*/
//	signed char		
Data.OutSFNote=atof("0");		/* note number alternative for OutSFreq	*/
//	char			
Data.pad1=0;

//	r_float			
Data.WaveFrequency=atof("0");	/* 0 if wavetable contains 1 cycle		*/
//	signed char		
Data.WaveFNote=atof("640");		/* note number alternative for WaveFreq	*/
//	char			pad2;

//	r_float			
Data.WaveSFrequency=atof("0");	/* sample frequency						*/
//	signed char		
Data.WaveSFNote;		/* note number alternative for WaveSFreq*/
//	char			pad3;

//	r_float			
Data.NoteLen=atof("5501");		/* yhden nuotin pituus näytteinä		*/

//	unsigned char	
Data.Waveform=0; /* 0 */
//	char			
Data.HPF=0;			/* high pass filter active				*/

//	r_float *		
Data.WaveTable=NULL;		/* osoite								*/
//	unsigned short	
Data.WaveLength=0;		/* pituus näytteinä						*/
//	r_float			
Data.PulseRatio=atof("5000");		/* sisäisen kanttiaallon pulssisuhde	*/

//	unsigned short	
Data.Length=4;			/* sekvenssin pituus nuotteina			*/

//	unsigned short	
Data.Repeats=1;		/* kertausten lukumäärä					*/

//	r_float			
Data.VCFamount=atof("10000");		/* wet/dry								*/

//	unsigned char	

//	r_float			
//Data.Cutoff=atof("11000");
//	signed char		
Data.CutoffNote=64-24;		/* note number alternative for Cutoff	*/

//	char			
Data.SustainF=0;		/* 1, jos halutaan sustainia, muuten 0	*/
//	r_float			
Data.Attack=atof("483");
//	r_float			
Data.Decay=atof("8200");
//	r_float			
Data.Sustain=atof("1");

//	r_float			
//Data.Release=atof("660");

//	r_float			
Data.ADSRFactor=atof("10000");
//	r_float			
Data.ResoFactor=atof("10000");
//	r_float			
Data.CutoffFactor=atof("10000");
//	r_float			

//	r_float			
Data.AmpModFactor=atof("10000");
//	r_float			
Data.PitchModFactor=atof("10000");
//	r_float			
Data.VolumeFactor=atof("10000");

//	r_float			
Data.EnvModAmp=atof("10000");
//	r_float			
//Data.EnvModVCF=atof("500");
//	r_float			
Data.EnvModPitch=atof("0");

//	r_float			
Data.Gain=atof("0");

//	r_float			
Data.Detune=atof("0");			//  1/100 -puolisävelaskelta		
//	r_float			
Data.Tuning=atof("44000");
//	signed char		
Data.Transpose=0;
// char
Data.pad6=0;
//	r_float			
Data.PortaTime=atof("2500");

// #endif


Data.Waveform=0;

//Data.Resonance=atof("10000");

//Data.CutoffFactor=atof("10000");

	DefaultTags("test.3MU");
	*TheData=Data;

	Data.VCFmodFactor=atof("5000");

	Data.VCFtype=1;		/* ks. alas								*/
	
//unsigned char	
Data.VCFpoles=2;

//r_float			
//Data.Resonance=atof("10000");



 return(1);
}



short *Test3MUSeq(struct Settings *TD,struct Note *N,int *Len) {
short *MyResult;
long len;
int ilen;

Data=*TD;


//	r_float			
Sequence[0].Frequency=atof("0");	/* vastaa siis Notea, TB laskee				*/
//	signed char		
Sequence[0].Note2=64-26;		/* nuotin MIDI-numero						*/

//	char			
Sequence[0].Mode=0;		/* legato, staccato, tie, rest yms.			*/
//	char			
Sequence[0].Accent=0;		/* 1=aksentti, 0=ei oo						*/
//	char			
Sequence[0].Porta=0;		/* 1=portamento, 0=ei oo					*/

//	r_float			
Sequence[0].Detune=0;
//	r_float			
Sequence[0].Attack=atof("0");
//	r_float			
Sequence[0].Decay=atof("0");
//	r_float			
Sequence[0].Sustain=atof("0");
//	r_float			
Sequence[0].Release=atof("0");
//	r_float			
Sequence[0].Resonance=atof("0");

//	r_float			
Sequence[0].Cutoff=atof("0");
//	signed char		
Sequence[0].CutoffNote=0;	/* note number alternative for Cutoff		*/
//	unsigned char	
Sequence[0].Length=atof("55");		/* 0% = release aivan nuotin alussa, 100% = tie w/trig*/

//	r_float			
//Sequence[0].Volume=atof("40");
//	r_float			
Sequence[0].EnvModVCF=0;
//	r_float			
Sequence[0].EnvModAmp=0;
//	r_float			
Sequence[0].EnvModPitch=0;

Sequence[0].Note2=N->Note2;
Sequence[0].Volume=N->Volume;
Sequence[0].Accent=N->Accent;
Sequence[0].Porta=N->Porta;


N++;

Sequence[1]=Sequence[0];
Sequence[1].Volume=N->Volume;
Sequence[1].Note2=N->Note2;
Sequence[1].Accent=N->Accent;
Sequence[1].Porta=N->Porta;
N++;

//Sequence[1].Cutoff=atof("402");

Sequence[2]=Sequence[0];
Sequence[2].Note2=N->Note2;
Sequence[2].Volume=N->Volume;
Sequence[2].Accent=N->Accent;
Sequence[2].Porta=N->Porta;

N++;

//Sequence[2].Cutoff=atof("202");

Sequence[3]=Sequence[0];
Sequence[3].Note2=N->Note2;
Sequence[3].Volume=N->Volume;
Sequence[3].Accent=N->Accent;
Sequence[3].Porta=N->Porta;

N++;

Sequence[4]=Sequence[0];
Sequence[4].Note2=N->Note2;
Sequence[4].Volume=N->Volume;
Sequence[4].Accent=N->Accent;
Sequence[4].Porta=N->Porta;

N++;

Sequence[5]=Sequence[0];
Sequence[5].Note2=N->Note2;
Sequence[5].Volume=N->Volume;
Sequence[5].Accent=N->Accent;
Sequence[5].Porta=N->Porta;

N++;

Sequence[6]=Sequence[0];
Sequence[6].Note2=N->Note2;
Sequence[6].Volume=N->Volume;
Sequence[6].Accent=N->Accent;
Sequence[6].Porta=N->Porta;

N++;

Sequence[7]=Sequence[0];
Sequence[7].Note2=N->Note2;
Sequence[7].Volume=N->Volume;
Sequence[7].Accent=N->Accent;
Sequence[7].Porta=N->Porta;

N++;


//Sequence[3].Cutoff=atof("22");


Data.Length=8;

Prep(&Data,Sequence);

// Prep(&Data,Sequence);

 len = TBlen( &Data, Sequence );
 *Len = len;

 MyResult=calloc(len+1000,sizeof(short));

 OlofsTB( &Data, Sequence,MyResult);

return (MyResult);
}




short *Test3MU(int *Len) {

short *MyResult;
long len;
int ilen;
/*
struct Settings	Data;
struct Note		Sequence[ MAXNOTES ];
*/

#ifdef NOSTRANGESTUFF

//	char *			
Data.IDstring=ENVNAME;

//	char *			
Data.OutFilename=NULL;	/* NULL indicates output to STDOUT		*/
//	char *			
Data.WaveFilename=NULL;	/* NULL indicates input from STDIN		*/
//	unsigned char	
Data.OutFileformat=0;
//	unsigned char	
Data.WaveFileformat=0;	/* waveform file format					*/

/* Frequency and Note MUST follow each other, frequency first */

//	r_float			
Data.OutSFrequency=atof("4410000");	/*  lähtönäytetaajuus					*/
//	signed char		
Data.OutSFNote=atof("0");		/* note number alternative for OutSFreq	*/
//	char			
Data.pad1=0;

//	r_float			
Data.WaveFrequency=atof("0");	/* 0 if wavetable contains 1 cycle		*/
//	signed char		
Data.WaveFNote=atof("640");		/* note number alternative for WaveFreq	*/
//	char			pad2;

//	r_float			
Data.WaveSFrequency=atof("0");	/* sample frequency						*/
//	signed char		
Data.WaveSFNote;		/* note number alternative for WaveSFreq*/
//	char			pad3;

//	r_float			
Data.NoteLen=atof("5501");		/* yhden nuotin pituus näytteinä		*/

//	unsigned char	
Data.Waveform=0; /* 0 */
//	char			
Data.HPF=0;			/* high pass filter active				*/

//	r_float *		
Data.WaveTable=NULL;		/* osoite								*/
//	unsigned short	
Data.WaveLength=0;		/* pituus näytteinä						*/
//	r_float			
Data.PulseRatio=atof("5000");		/* sisäisen kanttiaallon pulssisuhde	*/

//	unsigned short	
Data.Length=4;			/* sekvenssin pituus nuotteina			*/

//	unsigned short	
Data.Repeats=1;		/* kertausten lukumäärä					*/

//	r_float			
Data.VCFamount=atof("10000");		/* wet/dry								*/

//	unsigned char	
Data.VCFtype=1;		/* ks. alas								*/
	
//unsigned char	
Data.VCFpoles=2;

//r_float			
Data.Resonance=0;
//	r_float			
Data.Cutoff=atof("11000");
//	signed char		
Data.CutoffNote=0;		/* note number alternative for Cutoff	*/

//	char			
Data.SustainF=0;		/* 1, jos halutaan sustainia, muuten 0	*/
//	r_float			
Data.Attack=atof("483");
//	r_float			
Data.Decay=atof("8200");
//	r_float			
Data.Sustain=atof("1");

//	r_float			
Data.Release=atof("660");

//	r_float			
Data.ADSRFactor=atof("10000");
//	r_float			
Data.ResoFactor=atof("10000");
//	r_float			
Data.CutoffFactor=atof("10000");
//	r_float			
Data.VCFmodFactor=atof("100000");
//	r_float			
Data.AmpModFactor=atof("10000");
//	r_float			
Data.PitchModFactor=atof("10000");
//	r_float			
Data.VolumeFactor=atof("10000");

//	r_float			
Data.EnvModAmp=atof("10000");
//	r_float			
Data.EnvModVCF=atof("5000");
//	r_float			
Data.EnvModPitch=atof("0");

//	r_float			
Data.Gain=atof("0");

//	r_float			
Data.Detune=atof("0");			//  1/100 -puolisävelaskelta		
//	r_float			
Data.Tuning=atof("44000");
//	signed char		
Data.Transpose=0;
// char
Data.pad6=0;
//	r_float			
Data.PortaTime=atof("2500");






DefaultTags("test.3MU");

Data.Waveform=0;

Data.Resonance=atof("10000");

Data.CutoffFactor=atof("10000");

//	r_float			
Sequence[0].Frequency=atof("0");	/* vastaa siis Notea, TB laskee				*/
//	signed char		
Sequence[0].Note2=54;		/* nuotin MIDI-numero						*/

//	char			
Sequence[0].Mode=0;		/* legato, staccato, tie, rest yms.			*/
//	char			
Sequence[0].Accent=0;		/* 1=aksentti, 0=ei oo						*/
//	char			
Sequence[0].Porta=0;		/* 1=portamento, 0=ei oo					*/

//	r_float			
Sequence[0].Detune=0;
//	r_float			
Sequence[0].Attack=atof("0");
//	r_float			
Sequence[0].Decay=atof("0");
//	r_float			
Sequence[0].Sustain=atof("0");
//	r_float			
Sequence[0].Release=atof("0");
//	r_float			
Sequence[0].Resonance=atof("200");

//	r_float			
Sequence[0].Cutoff=atof("705");
//	signed char		
Sequence[0].CutoffNote=0;	/* note number alternative for Cutoff		*/
//	unsigned char	
Sequence[0].Length=atof("55");		/* 0% = release aivan nuotin alussa, 100% = tie w/trig*/

//	r_float			
Sequence[0].Volume=atof("40");
//	r_float			
Sequence[0].EnvModVCF=0;
//	r_float			
Sequence[0].EnvModAmp=0;
//	r_float			
Sequence[0].EnvModPitch=0;


Sequence[1]=Sequence[0];
Sequence[1].Note2=54-4;
Sequence[1].Cutoff=atof("402");

Sequence[2]=Sequence[0];
Sequence[2].Note2=54-2;
Sequence[2].Cutoff=atof("202");

Sequence[3]=Sequence[0];
Sequence[3].Note2=54-4;
Sequence[3].Cutoff=atof("22");


Data.Length=4;

Prep(&Data,Sequence);

 


 len = TBlen( &Data, Sequence );
 *Len = len;

 MyResult=calloc(len+1000,sizeof(short));




 OlofsTB( &Data, Sequence,MyResult);

#endif

return (MyResult);
}





















		/* ---		    Loppu		--- */
