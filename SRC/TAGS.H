/*
 *	tags.h			source tag numbers
 *
 *	Copyright © 1996 Jarno Seppänen; see 3MU.c for licensing info.
 */

#ifndef TAGS_H
#define TAGS_H 1

#define	END_TAG		" END"
#define	NULL_TAG	" NULL"

/* tag numbers: */

#define	T_NULL			0

#define	T_ATTACK		1
#define	T_CUT			2
#define	T_CUT_MOD		3
#define	T_OUT_FILEF		4
#define	T_OUT_FILEN		5
#define	T_OUT_SFREQ		6
#define	T_TEMPO			7	/* sequence tempo			*/
#define	T_DECAY			8
#define	T_DETUNE		9
#define	T_ENG_NAME		10
#define	T_ENG_VER		11
#define	T_FIL_POLES		12
#define	T_ID_STRING		13
/* 14,15,16 */
#define	T_NOTE			17
#define	T_N_ACCENT		18
#define	T_N_ADSR_F		19
#define	T_N_ATTACK		20
#define	T_N_CUT			21
#define	T_N_CUT_MOD		22
#define	T_N_CUT_MOD_F	23
#define	T_N_CUT_F		24
#define	T_N_DECAY		25
#define	T_N_DETUNE		26
#define	T_N_MODE		27
#define	T_N_PORTA		28
#define	T_N_RELEASE		29
#define	T_N_RESO		30
#define	T_N_RESO_F		31
#define T_N_LENGTH		32
#define	T_N_SUSTAIN		33
#define	T_N_VOL			34
#define	T_N_VOL_MOD		35
#define	T_N_VOL_MOD_F	36
#define	T_N_VOL_F		37
#define	T_PORTA_TIME	38
#define	T_RELEASE		39
#define	T_RESONANCE		40
#define	T_SEQ_LEN		41	/* sequence length in samples */
#define	T_REPEAT		42
#define	T_SUSTAIN		43
#define	T_TUNING		44
#define	T_GAIN			45
#define	T_VOL_MOD		46
#define	T_WAVEFORM		47
#define	T_NOTE_DUR		48	/* note duration			*/
#define T_NOTE_LEN		49	/* note length in samples	*/
#define T_ENV_NAME		50
#define T_ENV_VER		51
#define	T_TRANS			52
#define T_SEQ_DUR		53	/* sequence duration		*/	
#define	T_WAVE_FILEF	54
#define	T_WAVE_FILEN	55
#define	T_WAVE_FREQ		56
#define	T_WAVE_SFREQ	57
#define	T_WAVE_PRATIO	58	/* pulssisuhde				*/
#define	T_FIL_AMNT		59
#define T_OSC_HPF		60
#define T_PIT_MOD		61
#define	T_N_PIT_MOD		62
#define	T_N_PIT_MOD_F	63
#define	T_FIL_TYPE		64

#endif
