/*
 *	data.h			generic header file
 *
 *	Copyright © 1995, 1996 Jarno Seppänen; see 3MU.c for licensing info.
 */

#ifndef	_DATA_H
#define	_DATA_H 1

struct Settings
{
	char *			IDstring;

	char *			OutFilename;	/* NULL indicates output to STDOUT		*/
	char *			WaveFilename;	/* NULL indicates input from STDIN		*/
	unsigned char	OutFileformat;
	unsigned char	WaveFileformat;	/* waveform file format					*/

/* Frequency and Note MUST follow each other, frequency first */

	float			OutSFrequency;	/* lähtönäytetaajuus					*/
	signed char		OutSFNote;		/* note number alternative for OutSFreq	*/
	char			pad1;

	float			WaveFrequency;	/* 0 if wavetable contains 1 cycle		*/
	signed char		WaveFNote;		/* note number alternative for WaveFreq	*/
	char			pad2;

	float			WaveSFrequency;	/* sample frequency						*/
	signed char		WaveSFNote;		/* note number alternative for WaveSFreq*/
	char			pad3;

	float			NoteLen;		/* yhden nuotin pituus näytteinä		*/

	unsigned char	Waveform;
	char			HPF;			/* high pass filter active				*/
	float *		WaveTable;		/* osoite								*/
	unsigned short	WaveLength;		/* pituus näytteinä						*/
	float			PulseRatio;		/* sisäisen kanttiaallon pulssisuhde	*/

	unsigned short	Length;			/* sekvenssin pituus nuotteina			*/
	unsigned short	Repeats;		/* kertausten lukumäärä					*/

	float			VCFamount;		/* wet/dry								*/
	unsigned char	VCFtype;		/* ks. alas								*/
	unsigned char	VCFpoles;
	float			Resonance;
	float			Cutoff;
	signed char		CutoffNote;		/* note number alternative for Cutoff	*/

	char			SustainF;		/* 1, jos halutaan sustainia, muuten 0	*/
	float			Attack;
	float			Decay;
	float			Sustain;
	float			Release;

	float			ADSRFactor;
	float			ResoFactor;
	float			CutoffFactor;
	float			VCFmodFactor;
	float			AmpModFactor;
	float			PitchModFactor;
	float			VolumeFactor;

	float			EnvModAmp;
	float			EnvModVCF;
	float			EnvModPitch;

	float			Gain;

	float			Detune;			/* 1/100 -puolisävelaskelta				*/
	float			Tuning;
	signed char		Transpose;
	char			pad6;
	float			PortaTime;

	unsigned long	Reserved[8];
};


typedef struct Note	/* sekvenssi koostuu näistä */
{
/* Frequency and Note MUST follow each other, frequency first */

	float			Frequency;	/* vastaa siis Notea, TB laskee				*/
	signed char		Note2;		/* nuotin MIDI-numero						*/

	char			Mode;		/* legato, staccato, tie, rest yms.			*/
	char			Accent;		/* 1=aksentti, 0=ei oo						*/
	char			Porta;		/* 1=portamento, 0=ei oo					*/

	float			Detune;
	float			Attack;
	float			Decay;
	float			Sustain;
	float			Release;
	float			Resonance;

	float			Cutoff;
	signed char		CutoffNote;	/* note number alternative for Cutoff		*/
	unsigned char	Length;		/* 0% = release aivan nuotin alussa, 100% = tie w/trig*/

	float			Volume;
	float			EnvModVCF;
	float			EnvModAmp;
	float			EnvModPitch;

	unsigned long	Reserved[5];
} Note;


#endif	/* _data_H */
