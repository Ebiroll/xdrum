/* 
 * play.h
 * Not really used yet 
 * 
 */ 

#ifndef PLAYER
#define PLAYER

#define RECORD 0		/* mode: record */
#define PLAY 1			/* mode: play   */
#define AUDIO "/dev/dsp"
#define COMMANDLINE 0		/* called from commandline */
#define X11 1			/* called from xplay       */

typedef unsigned long DWORD;
typedef unsigned short WORD;

typedef struct {		/* header for WAV-Files */
	char main_chunk[4];	/* 'RIFF' */
	DWORD length;		/* length of file */
	char chunk_type[4];	/* 'WAVE' */
	char sub_chunk[4];	/* 'fmt' */
	DWORD length_chunk;	/* length sub_chunk, always 16 bytes */
	WORD format;		/* always 1 = PCM-Code */
	WORD modus;		/* 1 = Mono, 2 = Stereo */
	DWORD sample_fq;	/* Sample Freq */
	DWORD byte_p_sec;	/* Data per sec */
	WORD byte_p_spl;	/* bytes per sample, 1=8 bit, 2=16 bit (mono)
						     2=8 bit, 4=16 bit (stereo) */
	WORD bit_p_spl;		/* bits per sample, 8, 12, 16 */
	char data_chunk[4];	/* 'data' */
	DWORD data_length;	/* length of data */
} wave_header;

typedef struct {		/* options set */
	int quiet_mode;		/* -q */
	DWORD speed;		/* -s xxxxx */
	int force_speed;
	int stereo;		/* -S */
	int force_stereo;
	DWORD sample_size;	/* -b xx */
	int force_sample_size;
	float time_limit;	/* -t xxx */
	DWORD lower_border;
	DWORD upper_border;	/* for xplay */
} header_data;

/* prototype of error_handler */
void err(int severity, char *text);

#endif
