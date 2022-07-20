/*
 *	io.c			file i/o
 *
 *	Copyright © 1995, 1996 Jarno Seppänen; see 3MU.c for licensing info.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "3MU.h"
#include "console.h"
#include "convaud.h"
#include "debug.h"

#include "r_ffp/r_ffp.h"
#include "misc_pro.h"
#include "parse_pr.h"
#include "scrio_pr.h"
#include "seq_prot.h"

		/* ---	   Funktioiden prototyypit	--- */

long OpenFiles( void );
long CloseFiles( void );
long ReadSample( FILE *file, r_float *fin, unsigned char format );
long WriteSample( FILE *file, r_float fout, unsigned char format );
FILE *OpenSource( const char *srcname );

static long OpenIn( void );
static long OpenOut( void );
static long ReadWave( void );

static long FileLength( FILE *file );
static char *ChBufSize( FILE *file, size_t size );

r_float Limit( r_float sample );

		/* ---		  Muuttujat		--- */

BOOL	Query = 1;		/* kysytäänkö `Overwrite?' vai runtataanko	*/
size_t	IObufSize = 0;	/* uusi i/o-puskurin koko tai nolla			*/

static	char *rbuffer = NULL, *wbuffer = NULL;	/* puskurien osoitteet		*/

		/* ---		   Funktiot		--- */

/*		long OpenFiles( const char *srcname );
 *
 *	Avaa tiedostot. Palauttaa virhekoodin.
 */

long OpenFiles( void )
{
	if( OpenOut())
		return( -1 );
	if( OpenIn())
		return( -1 );
	if( ReadWave())
		return( -1 );
	return( 0 );
}

long CloseFiles( void )
{
	if( Data.WaveTable != NULL )
	{
#ifdef DEBUG
		DEBUGMSG( 2, "io: free wavetable mem" )
#endif
		free( Data.WaveTable );
	}
	if( Data.OutFilename != NULL && outfile != NULL )
	{
#ifdef DEBUG
		DEBUGMSG( 2, "io: close save file" )
#endif
		fclose( outfile );
		if( wbuffer != NULL )
		{
			free( wbuffer );
			wbuffer = NULL;
		}
	}
	if( Data.WaveFilename != NULL && infile != NULL )
	{
#ifdef DEBUG
		DEBUGMSG( 2, "io: close load file" )
#endif
		fclose( infile );
		if( rbuffer != NULL )
		{
			free( rbuffer );
			rbuffer = NULL;
		}
	}
	return( 0 );
}

/*		long ReadSample( FILE *file, r_float *fin, unsigned char format );
 *
 *	Lukee format-muodossa olevan ääninäytteen filestä.
 *	Tämä ääninäyte muunnetaan välillä [-1.0, 1.0]
 *	olevaksi liukuluvuksi ja sijoitetaan muuttujaan *fin.
 *	Palauttaa virhekoodin.
 */

long ReadSample( FILE *file, r_float *fin, unsigned char format )
{
	int read;
	short in;
	if( file == NULL )
		return( 2 );
	StopTimer();
	switch( format )
	{
		case FILE_BYTE:
			read = fgetc( file );
			if( read == EOF )	return( -1 );

			*fin = aud2ffp8( (signed char)read );
			break;

		case FILE_WORD:
			if( fread( &in, sizeof( short ), 1, file ) != 1 )
				return( -1 );

			/* big-endian byte order */
#if 0
	#ifdef INTEL
		swaphalves( &in );
	#endif
#endif
			*fin = aud2ffp16( in );
			break;

		case FILE_FLOAT:
			if( fread( fin, sizeof( r_float ), 1, file ) != 1 )
				return( -1 );
			/* let's restrict the somewhat infinite range of floats */
			*fin = Limit( *fin );
			break;

		default:
			fputs( "Bug: Unknown input file format.\n", stderr );
			return( 1 );
	}
	StartTimer();
	return( 0 );
}

/*		long WriteSample( FILE *file, r_float fout, unsiged char format );
 *
 *	Muuttaa välillä [-1.0, 1.0] olevan liukulukunäytteen fout
 *	format:n esittämään muotoon ja syöttää tuloksen file:n. Päivittää
 *	prosenttilaskurin.
 *	Palauttaa virhekoodin.
 */

long WriteSample( FILE *file, r_float fout, unsigned char format )
{
	short out;

	if( file == NULL )
		return( 2 );
	StopTimer();
	switch( format )
	{
		case FILE_BYTE:
			UpdPercent( sizeof( signed char ), fout );
			fout = Limit( fout );

			fputc( (signed char)ffp2aud8( fout ), file );
			if( ferror( file ) || feof( file ))
				return( -1 );
			break;

		case FILE_WORD:
			UpdPercent( sizeof( short ), fout );
			fout = Limit( fout );
			out = ffp2aud16( fout );

			/* big-endian byte order */
#if 0
	#ifdef INTEL
		swaphalves( &out );
	#endif
#endif
			if( fwrite( &out, sizeof( short ), 1, file ) != 1 )
				return( -1 );
			break;

		case FILE_FLOAT:
			UpdPercent( sizeof( r_float ), fout );
			/* no limiting in floats */
			if( fwrite( &fout, sizeof( r_float ), 1, file ) != 1 )
				return( -1 );
			break;

		default:
			fputs( "Bug: Unknown output file format.\n", stderr );
			return( 1 );
	}
	StartTimer();
	return( 0 );
}

/*		FILE *OpenSource( const char *srcname )
 *
 *	Avaa lähdekooditiedoston nimeltä *srcname.
 *	Palauttaa tiedosto-osoittimen tai NULL:n virhetapauksessa.
 */

FILE *OpenSource( const char *srcname )
{
	int sl = strlen( srcname );
	FILE *file = fopen( srcname, "r" );
	if( file == NULL )
	{
		char realname[ FILENAME_MAX ];
#ifndef MSDOS
		if(( sl >= 4 ) && !strcmp( &srcname[ sl - 4 ], ".3MU" ))
#else
		if(( sl >= 4 ) && CmpS( &srcname[ sl - 4 ], ".3MU" ))
#endif
		{
			fprintf( stderr, "File `%s' not found or it is in use.\n",
					srcname );
			return( NULL );
		}
		strncpy( realname, srcname, FILENAME_MAX );
		strncat( realname, ".3MU", FILENAME_MAX );
		file = fopen( realname, "r" );
		if( file == NULL )
		{
			fprintf( stderr,
					"File `%s' nor file `%s' not found or they are in use.\n",
					srcname, realname );
			return( NULL );
		}
	}
	return( file );
}

static long OpenIn( void )
{
	r_float fsize;	/* file size			*/
	long ifsize;	/* integer file size	*/
	if( Data.WaveFilename != NULL )
	{
#ifdef DEBUG
		DEBUGMSG( 2, "io: open waveform file" )
#endif
		infile = fopen( Data.WaveFilename, "r" );
		if( infile == NULL )
		{
			fprintf( stderr, "File %s not found.\n", Data.WaveFilename );
			return( -1 );
		}
		if( IObufSize && ( rbuffer = ChBufSize( infile, IObufSize )) == NULL )
			return( -1 );
		ifsize = FileLength( infile );
		if( ifsize == -1 )
			return( -1 );
		fsize = r_itof( ifsize );
		switch( Data.WaveFileformat )
		{
			case FILE_BYTE:
				fsize = r_div( fsize, r_itof( sizeof( signed char )));
				break;
			case FILE_WORD:
				fsize = r_div( fsize, r_itof( sizeof( short )));
				break;
			case FILE_FLOAT:
				fsize = r_div( fsize, r_itof( sizeof( r_float )));
				break;
			default:
				fputs( "Unknown wavetable file format.\n", stderr );
				return( -1 );
		}
		if( fsize == FP0 )
		{
			fputs( "Zero wavetable file size.\n", stderr );
			return( -1 );
		}
		Data.WaveLength = r_trunc( fsize );
		if( r_cmpable( fsize ) > r_cmpable( r_itof( Data.WaveLength )))
			Data.WaveLength++;
	}
	else
		infile = stdin;
	return( 0 );
}

static long OpenOut( void )
{
	if( Data.OutFilename != NULL )
	{
#ifdef DEBUG
		DEBUGMSG( 2, "io: open save file" )
#endif
		outfile = fopen( Data.OutFilename, "r" );
		if( outfile != NULL )
		{
			fclose( outfile );
			if( Verbose && Query )
			{
				printf( "Enter Y to overwrite existing file `%s': n" bwd( "1" ),
						Data.OutFilename );
				if( getchar() != 'Y' )
				{
					if
					getchar();	/* line feed */
					return( -1 );
				}
				getchar();		/* line feed */
			}
		}
		outfile = fopen( Data.OutFilename, "w" );
		if( outfile == NULL )
		{
			fprintf( stderr, "Could not open file `%s' for writing.\n",
						Data.OutFilename );
			return( -1 );
		}
		if( IObufSize && ( wbuffer = ChBufSize( outfile, IObufSize )) == NULL )
			return( -1 );
	}
	else
		outfile = stdout;
	return( 0 );
}

static long ReadWave( void )
{
	if( Data.Waveform == WAVE_ONE_SHOT || Data.Waveform == WAVE_LOOPED )
	{
		r_float *tab, *tend, *tptr;
		unsigned long tsize;

#ifdef DEBUG
		DEBUGMSG( 2, "io: load wavetable" )
#endif

		tsize = Data.WaveLength * sizeof( r_float );
		tab = malloc( tsize );
		if( tab == NULL )
		{
			fputs( "Not enough memory for wavetable.\n", stderr );
			return( -1 );
		}
		Data.WaveTable = tab;

		tend = (r_float *)((unsigned long)tab + tsize);
		for( tptr = tab; tptr < tend; tptr++ )
			if( ReadSample( infile, tptr, Data.WaveFileformat ))
			{
				fputs( "Couldn't read from wavetable file.\n", stderr );
				return( -1 );
			}
		if( Data.WaveFilename != NULL && infile != NULL )
		{
			fclose( infile );
			infile = NULL;
		}
	}
	return( 0 );
}

/*		long FileLength( FILE *file );
 *
 *	Palauttaa tiedoston *file pituuden tai -1:n, jos jotain meni pieleen.
 */

static long FileLength( FILE *file )
{
	long prev, size;
	prev = ftell( file );
	if( prev == -1 )
		return( -1 );
	if( fseek( file, 0L, SEEK_END ) == -1 )
		return( -1 );
	size = ftell( file );
	if( size == -1 )
		return( -1 );
	if( fseek( file, prev, SEEK_SET ) == -1 )
		return( -1 );
	return( size );
}

/*		char *ChBufSize( FILE *file, size_t size );
 *
 *	file on osoitin tiedostoon ja size on uuden puskurin koko.
 *	palauttaa osoittimen puskuriin.
 */

static char *ChBufSize( FILE *file, size_t size )
{
	if( size )
	{
		char *buf = malloc( size );
		if( buf == NULL )
		{
			fputs( "Not enough memory for I/O buffer.\n", stderr );
			return( NULL );
		}
		if( setvbuf( file, buf, _IOFBF, size ))
		{
			free( buf );
			fputs( "Couldn't change I/O buffer size.\n", stderr );
			return( NULL );
		}
		return( buf );
	}
	return( NULL );
}

/*		static r_float Limit( r_float sample );
 *
 *	Rajoittaa samplen välille [-1.0, 1.0]
 */

r_float Limit( r_float sample )
{
	/* *hard* limiting aka clipping */
	if( r_cmpable( r_fabs( sample )) <= r_cmpable( FP1 ))
		return( sample );

	if( r_sign( sample ))
		return( FP_1 );
	return( FP1 );
}

		/* ---		    Loppu		--- */
