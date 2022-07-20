/*
 *	taglist.c		source tag definition list
 *
 *	Copyright © 1996 Jarno Sepp‰nen; see 3MU.c for licensing info.
 */

#include <stddef.h>
#ifdef NULL
#undef NULL
#endif
#define NULL ((void *)0L)		/* stddef.h:ssa m‰‰ritell‰‰n pelk‰ksi 0L:ksi! */

#include "3MU.h"
#include "tags.h"

#include "misc_pro.h"

#define NOFFSET(l)	(void *)offsetof( struct Note, l )

		/* ---	Ulkoisten muuttujien esittelyt	--- */

extern r_float NLvalue;	/* parse.c */

		/* ---		  Muuttujat		--- */

struct Tag Tags[] =
{{	NULL_TAG,						T_NULL, 0, { NULL }, 0, NULL },

{	"Attack",						T_ATTACK	,	0, { &Data.Attack },		VAR_FLOAT,	"4,83" },
{	"Cutoff",						T_CUT		,	0, { &Data.Cutoff },		VAR_FREQ,	"160" },
{	"Cutoff-EnvMod",				T_CUT_MOD	,	0, { &Data.EnvModVCF },		VAR_FLOAT,	"0" },
{	"Decay",						T_DECAY		,	0, { &Data.Decay },			VAR_FLOAT,	"120" },
{	"Detune",						T_DETUNE	,	0, { &Data.Detune },		VAR_FLOAT,	"0" },
{	"Detune-EnvMod",				T_PIT_MOD,		0, { &Data.EnvModPitch },	VAR_FLOAT,	"0" },
{	"Engine-Name",					T_ENG_NAME	,	0, { &ReqEngName },			VAR_TEXT,	NULL },
{	"Engine-Version",				T_ENG_VER	,	0, { &ReqEngVers },			VAR_TEXT,	NULL },
{	"Environment-Name",				T_ENV_NAME	,	0, { &ReqEnvName },			VAR_TEXT,	NULL },
{	"Environment-Version",			T_ENV_VER	,	0, { &ReqEnvVers },			VAR_TEXT,	NULL },
{	"Filter-Amount",				T_FIL_AMNT,		0, { &Data.VCFamount },		VAR_FLOAT,	"100" },
{	"Filter-Poles",					T_FIL_POLES	,	0, { &Data.VCFpoles },		VAR_UBYTE,	"2" },
{	"Filter-Type",					T_FIL_TYPE,		0, { &Data.VCFtype },		VAR_CUSTOM,	FILTNAME_2POLE },
{	"Gain",							T_GAIN		,	0, { &Data.Gain },			VAR_FLOAT,	"0" },
{	"ID-String",					T_ID_STRING	,	0, { &Data.IDstring },		VAR_TEXT,	NULL },
{	"Note",							T_NOTE		,	1, { NOFFSET( Frequency )},VAR_FREQ,	"0" },
{	"Note-Accent",					T_N_ACCENT	,	1, { NOFFSET( Accent )},	VAR_UBYTE,	NULL },
{	"Note-ADSR-Factor",				T_N_ADSR_F	,	0, { &Data.ADSRFactor },	VAR_FLOAT,	"100" },
{	"Note-Attack",					T_N_ATTACK	,	1, { NOFFSET( Attack )},	VAR_FLOAT,	NULL },
{	"Note-Cutoff",					T_N_CUT		,	1, { NOFFSET( Cutoff )},	VAR_FREQ,	"0" },
{	"Note-Cutoff-EnvMod",			T_N_CUT_MOD	,	1, { NOFFSET( EnvModVCF )},VAR_FLOAT,	NULL },
{	"Note-Cutoff-EnvMod-Factor",	T_N_CUT_MOD_F,	0, { &Data.VCFmodFactor },	VAR_FLOAT,	"100" },
{	"Note-Cutoff-Factor",			T_N_CUT_F	,	0, { &Data.CutoffFactor },	VAR_FLOAT,	"100" },
{	"Note-Decay",					T_N_DECAY	,	1, { NOFFSET( Decay )},	VAR_FLOAT,	NULL },
{	"Note-Detune",					T_N_DETUNE,		1, { NOFFSET( Detune )},	VAR_FLOAT,	NULL },
{	"Note-Detune-EnvMod",			T_N_PIT_MOD,	1, { NOFFSET(EnvModPitch)},VAR_FLOAT,	NULL },
{	"Note-Detune-EnvMod-Factor",	T_N_PIT_MOD_F,	0, { &Data.PitchModFactor },VAR_FLOAT,	"100" },
{	"Note-Duration",				T_NOTE_DUR,		0, { &NLvalue },			VAR_NLEN,	NULL },
{	"Note-Length",					T_N_LENGTH,		1, { NOFFSET( Length )},	VAR_CUSTOM,	"55" },
{	"Note-Mode",					T_N_MODE	,	1, { NOFFSET( Mode )},		VAR_CUSTOM,	MNAME_NORMAL },
{	"Note-Portamento",				T_N_PORTA	,	1, { NOFFSET( Porta )},	VAR_UBYTE,	NULL },
{	"Note-Release",					T_N_RELEASE	,	1, { NOFFSET( Release )},	VAR_FLOAT,	NULL },
{	"Note-Resonance",				T_N_RESO	,	1, { NOFFSET( Resonance )},VAR_FLOAT,	NULL },
{	"Note-Resonance-Factor",		T_N_RESO_F	,	0, { &Data.ResoFactor },	VAR_FLOAT,	"100" },
{	"Note-Sample-Length",			T_NOTE_LEN	,	0, { &NLvalue },			VAR_NLEN,	NULL },
{	"Note-Sustain",					T_N_SUSTAIN	,	1, { NOFFSET( Sustain )},	VAR_FLOAT,	NULL },
{	"Note-Volume",					T_N_VOL		,	1, { NOFFSET( Volume )},	VAR_FLOAT,	"100" },
{	"Note-Volume-EnvMod",			T_N_VOL_MOD	,	1, { NOFFSET( EnvModAmp )},VAR_FLOAT,	NULL },
{	"Note-Volume-EnvMod-Factor",	T_N_VOL_MOD_F,	0, { &Data.AmpModFactor },	VAR_FLOAT,	"100" },
{	"Note-Volume-Factor",			T_N_VOL_F	,	0, { &Data.VolumeFactor },	VAR_FLOAT,	"100" },
{	"Oscillator-HPF",				T_OSC_HPF,		0, { &Data.HPF },			VAR_TOGGLE,	"off" },
{	"Output-Fileformat",			T_OUT_FILEF	,	0, { &Data.OutFileformat },	VAR_CUSTOM,	FFNAME_BYTE },
{	"Output-Filename",				T_OUT_FILEN	,	0, { &Data.OutFilename },	VAR_TEXT,	NULL },
{	"Output-Sample-Frequency",		T_OUT_SFREQ	,	0, { &Data.OutSFrequency },	VAR_FREQ,	"44100" },
{	"Portamento-Time",				T_PORTA_TIME,	0, { &Data.PortaTime },		VAR_FLOAT,	"25" },
{	"Release",						T_RELEASE	,	0, { &Data.Release },		VAR_FLOAT,	"6,96" },
{	"Repetitions",					T_REPEAT	,	0, { &Data.Repeats },		VAR_UWORD,	"1" },
{	"Resonance",					T_RESONANCE	,	0, { &Data.Resonance },		VAR_FLOAT,	"5" },
{	"Sequence-Duration",			T_SEQ_DUR,		0, { &NLvalue },			VAR_NLEN,	NULL },
{	"Sequence-Sample-Length",		T_SEQ_LEN,		0, { &NLvalue },			VAR_NLEN,	NULL },
{	"Sustain",						T_SUSTAIN	,	0, { &Data.Sustain },		VAR_CUSTOM,	NULL },
{	"Tempo",						T_TEMPO		,	0, { &NLvalue },			VAR_NLEN,	"120" },
{	"Transpose",					T_TRANS,		0, { &Data.Transpose },		VAR_UBYTE,	"0" },
{	"Tuning",						T_TUNING	,	0, { &Data.Tuning },		VAR_FLOAT,	"440" },
{	"Volume-EnvMod",				T_VOL_MOD	,	0, { &Data.EnvModAmp },		VAR_FLOAT,	"100" },
{	"Waveform",						T_WAVEFORM	,	0, { &Data.Waveform },		VAR_CUSTOM,	WFNAME_SQUARE },
{	"Waveform-Fileformat",			T_WAVE_FILEF,	0, { &Data.WaveFileformat },VAR_CUSTOM,	FFNAME_BYTE },
{	"Waveform-Filename",			T_WAVE_FILEN,	0, { &Data.WaveFilename },	VAR_TEXT,	NULL },
{	"Waveform-Frequency",			T_WAVE_FREQ,	0, { &Data.WaveFrequency },	VAR_FREQ,	"0" },
{	"Waveform-PulseRatio",			T_WAVE_PRATIO,	0, { &Data.PulseRatio },	VAR_FLOAT,	"50" },
{	"Waveform-Sample-Frequency",	T_WAVE_SFREQ,	0, { &Data.WaveSFrequency },VAR_FREQ,	"0" },

{	END_TAG,						T_NULL, 0, { NULL }, 0, NULL }};

		/* ---		    Loppu		--- */
