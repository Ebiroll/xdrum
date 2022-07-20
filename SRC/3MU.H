/*
 *	3MU.h			generic header file
 *
 *	Copyright © 1995, 1996 Jarno Sepp‰nen; see 3MU.c for licensing info.
 */

#ifndef	_3MU_H
#define	_3MU_H 1

#include "data.h"
#include "r_ffp/r_ffp.h"
#include "r_ffp/types.h"

/* ohjelman moottorin ja ymp‰ristˆn tunnusmerkit; nimi ja versio */

#define ENGNAME		"TB-303"
#define ENGVERS		"1.0js"
#define	ENVNAME		"3MU"
#define	ENVVERS		"1.1"

#ifdef _DCC
#define ILLEGALGAIN	(0x9502F962)
#define	ATTACKC		0xA3D70A3A		/* attack-vakio = 0.01	*/
#else
#define	ILLEGALGAIN	(1E10)
#define	ATTACKC		(0.01)
#endif

/* sekvenssin maksimipituus, sis‰‰nkoodattu */
#define	MAXNOTES	100		/* max 100 nuottia */

/* clock():ien m‰‰r‰ sekunnissa */
#ifdef AMIGA
#ifndef CLOCKS_PER_SEC
#define	CLOCKS_PER_SEC 50
#endif
#endif

/* Globaalit muuttujat sis‰lt‰v‰ tietue */

#ifdef NODATAHERE
struct Settings
{
	char *			IDstring;

	char *			OutFilename;	/* NULL indicates output to STDOUT		*/
	char *			WaveFilename;	/* NULL indicates input from STDIN		*/
	unsigned char	OutFileformat;
	unsigned char	WaveFileformat;	/* waveform file format					*/

/* Frequency and Note MUST follow each other, frequency first */

	r_float			OutSFrequency;	/* l‰htˆn‰ytetaajuus					*/
	signed char		OutSFNote;		/* note number alternative for OutSFreq	*/
	char			pad1;

	r_float			WaveFrequency;	/* 0 if wavetable contains 1 cycle		*/
	signed char		WaveFNote;		/* note number alternative for WaveFreq	*/
	char			pad2;

	r_float			WaveSFrequency;	/* sample frequency						*/
	signed char		WaveSFNote;		/* note number alternative for WaveSFreq*/
	char			pad3;

	r_float			NoteLen;		/* yhden nuotin pituus n‰yttein‰		*/

	unsigned char	Waveform;
	char			HPF;			/* high pass filter active				*/
	r_float *		WaveTable;		/* osoite								*/
	unsigned short	WaveLength;		/* pituus n‰yttein‰						*/
	r_float			PulseRatio;		/* sis‰isen kanttiaallon pulssisuhde	*/

	unsigned short	Length;			/* sekvenssin pituus nuotteina			*/
	unsigned short	Repeats;		/* kertausten lukum‰‰r‰					*/

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

	r_float			Detune;			/* 1/100 -puolis‰velaskelta				*/
	r_float			Tuning;
	signed char		Transpose;
	char			pad6;
	r_float			PortaTime;

	unsigned long	Reserved[8];
};

#endif
/* *FileFormat */
#define FFNAME_BYTE		"8-Bit"
#define FFNAME_WORD		"16-Bit"
#define FFNAME_FLOAT	"Float"
#define	FILE_BYTE		0
#define	FILE_WORD		1
#define	FILE_FLOAT		2

/* Waveform */
#define WFNAME_SQUARE		"Square"		/* internal square emu		*/
#define	WFNAME_SAWTOOTH		"Sawtooth"		/* int. saw emu				*/
#define	WFNAME_INPUT		"Input"			/* input from external file	*/
#define	WFNAME_LOOPED		"Looped"		/* looped external file		*/
#define	WFNAME_ONE_SHOT		"One-shot"		/* one-shot external file	*/
#define	WAVE_SAWTOOTH		0
#define	WAVE_SQUARE			1
#define	WAVE_INPUT			2
#define	WAVE_ONE_SHOT		3
#define	WAVE_LOOPED			4

/* VCFtype */
#if 0
#define	FILTNAME_LPF		"LPF"	/* low pass filter		*/
#define	FILTNAME_HPF		"HPF"	/* high pass filter		*/
#define	FILTNAME_BPF		"BPF"	/* band pass filter		*/
#define	FILTNAME_BRF		"BSF"	/* band reject/stop filter	*/
#define	FILT_LPF			0
#define	FILT_HPF			1
#define	FILT_BPF			2
#define	FILT_BRF			3
#else
#define FILTNAME_HAMRE      "Hamre"   /* orig filt by Lars Hamre */
#define FILTNAME_2POLE      "Generic" /* A generic 2-pole LPF    */
#define FILT_HAMRE          0
#define FILT_2POLE          1
#endif

/* Tag m‰‰rittelee yhden synteesi‰ ohjaavan parametrin;
 * taglist.c sis‰lt‰‰ varsinaiset m‰‰rittelyt
 */

struct Tag
{
	char *          Label;		/* teksti								*/
	unsigned short	Number;		/* sis‰inen viitenumero					*/
	BOOL			Sequential;	/* onko sekvenssinen muuttuja			*/
	union {
		void *			Variable;	/* sijoituspaikan osoite				*/
		unsigned long	Offset;		/* poikkeutus Note-struktuurin alusta	*/
	}				Location;	/* arvojen sijoituspaikka				*/
	unsigned short	Type;		/* arvon laatu, ks. alla				*/
	char *          Default;	/* oletusarvo							*/
};

#define	VAR_NULL	0
#define	VAR_TEXT	1	/* osoitin merkkijonoon tai mjono-osoittimeen	*/
						/* HUOM! VAR_TEXT-muuttujien k‰sittely ainoastaan */
						/* SetTextVar():lla								*/
#define	VAR_UWORD	2	/* uword tai osoitin uwordiin					*/
#define	VAR_UBYTE	3	/* ubyte tai osoitin ubyteen					*/
#define	VAR_FLOAT	4	/* float tai osoitin floatiin					*/
#define VAR_CUSTOM	5	/* k‰sitell‰‰n `k‰sityˆn‰'						*/
#define	VAR_TOGGLE	6	/* k‰‰nt‰‰ boolelaisen tavun arvon				*/
#define	VAR_NLEN	7	/* nuotin pituus, k‰sitell‰‰n erikseen			*/
#define	VAR_FREQ	8	/* taajuus (abs./suht.): sijaitsevat per‰kk‰in, */
						/* abs. taajuus on liukuluku ja suht. taajuus   */
						/* signed char, MIDI-nuotin numero				*/

/* Note on tietue, joka sis‰lt‰‰ sekvenssin yhden j‰senen tiedot */

#ifdef NODATAHERE

/* monet eri kent‰t voisi korvata lippukent‰ll‰... */
struct Note	/* sekvenssi koostuu n‰ist‰ */
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

#define	MNAME_NORMAL	"Normal"
#define	MNAME_REST		"Rest"
#define	MNAME_TIE		"Tie"
#define	MODE_NORMAL		0
#define	MODE_REST		1
#define	MODE_TIE		2
#define LNAME_NORMAL	"Normal"
#define	LNAME_LEGATO	"Legato"
#define	LNAME_STACCATO	"Staccato"
#define	LNAME_PORTATO	"Portato"
								/* release-arvot `eripituisille' nuoteille	*/
#define LEN_NORMAL		55		/* normaali: kun 55% nuotista kulunut		*/
#define	LEN_LEGATO		80		/* legato = 80%								*/
#define	LEN_STACCATO	25		/* staccato = 25%							*/
#define LEN_PORTATO		40		/* portato = 40%							*/


#endif	/* _3MU_H */
