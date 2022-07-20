/*
 *	io.c		file i/o
 *
 *	Copyright © 1995, 1996 Jarno Seppänen; see 3MU.c for licensing info.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#if 0
#include <dos/dos.h>
#endif

#include "3MU.h"
#include "con.h"
#include "convaud.h"
#include "r_ffp.h"
#include "misc_protos.h"
#include "parse_protos.h"
#include "tb_protos.h"

#include "debug.h"


#if 0
/* <clib/dos_protos.h> */
extern BPTR Lock( unsigned char *name, long type );
extern void UnLock( BPTR lock );
extern long Examine( BPTR lock, struct FileInfoBlock *fileInfoBlock );
#endif

		/* ---	   Funktioiden prototyypit	--- */

long OpenFiles( const char *srcname );
long CloseFiles( void );
long ReadSample( FILE *file, r_float *fin, unsigned char format );
long WriteSample( FILE *file, r_float fout, unsigned char format );
FILE *OpenSource( const char *srcname );

static long FileLength( FILE *file );
static char *ChBufSize( FILE *file, size_t size );

		/* ---		  Muuttujat		--- */

BOOL	Query = 1;		/* kysytäänkö `Overwrite?' vai runtataanko	*/
size_t	IObufSize = 0;	/* uusi i/o-puskurin koko tai nolla			*/

static	char *rbuffer = NULL, *wbuffer = NULL;	/* puskurien osoitteet		*/

		/* ---		   Funktiot		--- */

/*		long OpenFiles( const char *srcname );
 *
 *	Avaa tiedostot. Palauttaa virhekoodin.
 */

long OpenFiles( const char *srcname )
{
	if( Data.OutFilename == NULL )
	{
		/* asetetaan lähtötiedoston nimeksi lähdekooditiedoston nimi, jonka
			tarkennin on korvattu .snd:llä */
		int sl = strlen( srcname );
		char outname[ FILENAME_MAX ];
		strncpy( outname, srcname, FILENAME_MAX );
		if(( sl >= 4 ) && !strcmp( &srcname[ sl - 4 ], ".3MU" ))
			outname[ sl - 4 ] = 0;
		strncat( outname, ".snd", FILENAME_MAX );
		SetTextVar( &Data.OutFilename, outname );
	}
	r_float fsize;
	long ifsize;
	if( Data.InFilename )
	{
#ifdef DEBUG
		DEBUGMSG( 2, "io: open load file" )
#endif
		infile = fopen( Data.InFilename, "r" );
		if( infile == NULL )
		{
			fprintf( stderr, "File %s not found.\n", Data.InFilename );
			return( -1 );
		}
		if( IObufSize && ( rbuffer = ChBufSize( infile, IObufSize )) == NULL )
			return( -1 );
		ifsize = FileLength( infile );
		if( ifsize == -1 )
			return( -1 );
		fsize = r_itof( ifsize );
		switch( Data.InFileformat )
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
		}
		Data.InLength = fsize;
	}
	else
		infile = stdin;
	if( Data.OutFilename )
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

	if( Data.WaveFilename != NULL )
	{
#ifdef DEBUG
		DEBUGMSG( 2, "io: open wavetable file" )
#endif
		FILE *wavef = fopen( Data.WaveFilename, "r" );
		if( wavef == NULL )
		{
			fprintf( stderr, "File %s not found.\n", Data.WaveFilename );
			return( -1 );
		}
		ifsize = FileLength( wavef );
		if( ifsize == -1 )
		{
			fclose( wavef );
			return( -1 );
		}
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
				fclose( wavef );
				return( -1 );
		}
		if( fsize == FP0 )
		{
			fputs( "Zero wavetable file size (an error).\n", stderr );
			fclose( wavef );
			return( -1 );
		}
		Data.WaveLength = r_trunc( fsize );
		if( r_cmpable( fsize ) > r_cmpable( r_itof( Data.WaveLength )))
			Data.WaveLength++;

		unsigned long tsize = Data.WaveLength * sizeof( r_float );
		r_float *tab = malloc( tsize );
		if( tab == FP0 )
		{
			fputs( "Not enough memory for wavetable.\n", stderr );
			fclose( wavef );
			return( -1 );
		}
		Data.WaveTable = tab;

		r_float *tend = (r_float *)((unsigned long)tab + tsize);
		for( r_float *tptr = tab; tptr < tend; tptr++ )
			if( ReadSample( wavef, tptr, Data.WaveFileformat ))
			{
				fclose( wavef );
				fputs( "Couldn't read from wavetable file.\n", stderr );
				return( -1 );
			}
		fclose( wavef );
	}
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
	if(( Data.OutFilename != NULL ) && ( outfile != NULL ))
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
	if(( Data.InFilename != NULL ) && ( infile != NULL ))
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
	if( file == NULL )
		return( 2 );
	StopTimer();
	int read;
	switch( format )
	{
		case FILE_BYTE:
			read = fgetc( file );
			if( read == EOF )	return( -1 );

			*fin = aud2ffp8( (signed char)read );
			break;

		case FILE_WORD:
			short in;
			/* big-endian byte order */
			read = fgetc( file );
			if( read == EOF )	return( -1 );
			in = (short)read << 8;
			read = fgetc( file );
			if( read == EOF )	return( -1 );

			*fin = aud2ffp16( in | (short)read );
			break;

		case FILE_FLOAT:
			/* big-endian byte order */
			read = fgetc( file );
			if( read == EOF )	return( -1 );
			*fin = (r_float)read << 24;
			read = fgetc( file );
			if( read == EOF )	return( -1 );
			*fin = *fin | ( (r_float)read << 16 );
			read = fgetc( file );
			if( read == EOF )	return( -1 );
			*fin = *fin | ( (r_float)read << 8 );
			read = fgetc( file );
			if( read == EOF )	return( -1 );
			*fin = *fin | (r_float)read;
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
	if( file == NULL )
		return( 2 );
	StopTimer();
	switch( format )
	{
		case FILE_BYTE:
			signed char out = ffp2aud8( fout );
			if( out > CHAR_MAX )
				out = CHAR_MAX;
			if( out < CHAR_MIN )
				out = CHAR_MIN;

			fputc( out, file );
			if( ferror( file ) || feof( file ))
				return( -1 );
			UpdPercent( sizeof( signed char ));
			break;

		case FILE_WORD:
			short out = ffp2aud16( fout );
			if( out > SHRT_MAX )
				out = SHRT_MAX;
			if( out < SHRT_MIN )
				out = SHRT_MIN;

			/* big-endian byte order */
			fputc( (char)( out >> 8 ), file );
			fputc( (char)out, file );
			if( ferror( file ) || feof( file ))
				return( -1 );
			UpdPercent( sizeof( short ));
			break;

		case FILE_FLOAT:
			/* big-endian byte order */
			fputc( (char)( fout >> 24 ), file );
			fputc( (char)( fout >> 16 ), file );
			fputc( (char)( fout >> 8 ), file );
			fputc( (char)fout, file );
			if( ferror( file ) || feof( file ))
				return( -1 );
			UpdPercent( sizeof( r_float ));
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
	FILE *file;
	int sl = strlen( srcname );
	file = fopen( srcname, "r" );
	if( file == NULL )
	{
		if(( sl >= 4 ) && !strcmp( &srcname[ sl - 4 ], ".3MU" ))
		{
			fprintf( stderr, "File `%s' not found or it is in use.\n",
					srcname );
			return( NULL );
		}
		char realname[ FILENAME_MAX ];
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

/*		long FileLength( FILE *file );
 *
 *	Palauttaa tiedoston *file pituuden tai -1:n, jos jotain meni pieleen.
 */

static long FileLength( FILE *file )
{
	long prev = ftell( file );
	if( prev == -1 )
		return( -1 );
	if( fseek( file, 0L, SEEK_END ) == -1 )
		return( -1 );
	long size = ftell( file );
	if( size == -1 )
		return( -1 );
	if( fseek( file, prev, SEEK_SET ) == -1 )
		return( -1 );
	return( size );
}

#if 0
/* obsolescent, strictly-Amiga version: */
static unsigned long FileLength( const char *filename )
{
	if( filename == NULL )
		return( 0L );

	BPTR lock = Lock( filename, ACCESS_READ );
	if( lock == NULL )
		return( 0L );

	void *fibmem = malloc( sizeof( struct FileInfoBlock ) + 3 );/* MEMF_PUBLIC */
	if( fibmem == NULL )
	{
		UnLock( lock );
		fprintf( stderr, "Out of memory (%ld bytes needed).\n",
			sizeof( struct FileInfoBlock ) + 3 );
		return( 0L );
	}

	/* fib:n pitää olla longword-aligned l. 4:llä jaollinen */

	struct FileInfoBlock *fib = (struct FileInfoBlock *)
		((unsigned long)(fibmem + 3) & ~3L);

	if( !Examine( lock, fib ))
	{
		fprintf( stderr, "Could not examine file `%s'.\n", filename );
		free( fibmem );
		UnLock( lock );
		return( 0L );
	}
	UnLock( lock );
#if 0
	if( fib->fib_Protection & FIBF_READ )
	{
		fprintf( stderr, "File `%s' is not readable.\n", realname );
		free( fibmem );
		return( 0L );
	}
#endif
	unsigned long size = (unsigned long)( fib->fib_Size );
	free( fibmem );

	return( size );
}
#endif

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

		/* ---		    Loppu		--- */
