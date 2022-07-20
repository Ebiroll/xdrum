/*
 *	vca.c			voltage-controlled amplifier emulation
 *
 *	Copyright © 1996 Jarno Seppänen; see 3MU.c for licensing info.
 */

#include "r_ffp/r_ffp.h"

		/* ---	   Funktioiden prototyypit	--- */

r_float VCA( r_float inp, r_float env, r_float volume );

		/* ---		   Funktiot		--- */

/*		r_float VCA( r_float inp, r_float env, r_float volume )
 *
 *	Jänniteohjatun vahvistimen simulaatio.
 *	inp on tulo, env on vahvistustulo, volume yleinen voimakkuus.
 *	palauttaa lähtevän näytteen.
 */

r_float VCA( r_float inp, r_float env, r_float volume )
{
/* quite an ideal op-amp, huh */
	return( r_mul( r_mul( inp, env ), volume ));
}
		/* ---		    Loppu		--- */
