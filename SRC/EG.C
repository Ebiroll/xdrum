/*
 *	eg.c			envelope generator emulation
 *
 *	Copyright © 1996 Jarno Seppänen; see 3MU.c for licensing info.
 */

#include "3MU.h"
#include "debug.h"

#include "r_ffp/r_ffp.h"

#ifdef _DCC
#define	ACCENT		0x8CCCCD41	/* aksentti on 1.1 eli +10% */
#define	SUSCOEFF	afp( "0.9999" )
#else
#define	ACCENT		(1.1)
#define	SUSCOEFF	(0.9999)
#endif
#define FP20		r_itof( 20 )

		/* ---	   Funktioiden prototyypit	--- */

r_float EG( r_float a, r_float d, r_float s, r_float r,
			char acc, char len, BOOL trig, char nextmode, r_float time );

		/* ---		   Funktiot		--- */

/*	r_float EG( r_float a, r_float d, r_float s, r_float r,
 *			char acc, char len, BOOL trig, char nextmode, r_float time )
 *
 *	Tuottaa verhokäyrän. a, d ja r ovat rekursiiviset kertoimet, s
 *	on sustain level ]0, 1]. Ellei sustainia haluta, s=0
 *	acc on aksentti, len on decay+sustain-osan pituus prosentteina nuotin
 *	pituudesta [0, 100]. trig liipaisee verhokäyrän; nextmode on
 *	seuraavan nuotin moodi. time on nuotin osa [0, 1[
 *	Palauttaa uuden verhokäyrän arvon.
 */
 
r_float EG( r_float a, r_float d, r_float s, r_float r,
			char acc, char len, BOOL trig, char nextmode, r_float time )
{
	static int emode = 3;	/* release */
	static r_float amp = FP0;

	/* Trig envelope attack		*/
	if( trig )
	{
#ifdef DEBUG
		DEBUGMSG( 10, "attack" )
#endif
		emode = 0;
	}

	else if( nextmode != MODE_TIE )
	{
	/* Trig envelope sustain	*/
		if( emode == 1 && ( r_cmpable( amp ) <= r_cmpable( s )))
		{
#ifdef DEBUG
		DEBUGMSG( 10, "sustain" )
#endif
			emode = 2;
			amp = s;
		}

	/* Trig envelope release	*/
		else if( emode != 3 &&
			r_cmpable( time ) >= r_cmpable( r_div( r_itof( len ), FP1E2 )))
		{
#ifdef DEBUG
		DEBUGMSG( 10, "release" )
#endif
			emode = 3;
		}
	}

	switch( emode )
	{
		case 0:
			/* Attack state */

			/* Capacitor charge attack would be
			 *  amp = amp + (1.1 - amp) * 0.01
			 * but the TB-303 attack looks more like
			 *  amp = 1.1 * amp + 0.01
			 */
			amp = r_add( r_mul( amp, a ), ATTACKC );

			if( acc )	/* Check accent and attack->decay transition */
			{
				if( r_cmpable( amp ) >= r_cmpable( ACCENT ))
				{
					amp = ACCENT;
					emode = 1;	/* decay */
				}
			} else
			{
				if( r_cmpable( amp ) >= r_cmpable( FP1 ))
				{
					amp = FP1;
					emode = 1;	/* decay */
				}
			}
#ifdef DEBUG
			if( emode == 1 )
				DEBUGMSG( 10, "decay" )
#endif
			break;

		/* Decay state - tweak as you like */
		case 1:
			amp = r_mul( amp, d );
			break;

		/* Sustain state
		 * not found on TB-303s on planet Earth...
		 */
		case 2:
			amp = r_mul( amp, SUSCOEFF );
			break;

		/* Release state */
		case 3:
			amp = r_mul( amp, r );
			break;
	}
	return( amp );
}

		/* ---		    Loppu		--- */
