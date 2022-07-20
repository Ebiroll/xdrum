/*
 *	vco.c			voltage-controlled oscillator emulation
 *
 *	Copyright © 1996 Jarno Seppänen; see 3MU.c for licensing info.
 */

#include <stdio.h>

#include "3MU.h"
#include "r_ffp/r_ffp.h"

		/* ---	   Funktioiden prototyypit	--- */

long VCOinit( char oscmode, char hpfilt,
				unsigned short tlen, r_float *taddr, r_float tcycles,
				r_float pratio, r_float initrfreq, r_float portalength,
				r_float noportalength );
r_float VCO( r_float rfreq, r_float env, BOOL porta, BOOL trig );
static r_float O303( r_float rphase, r_float rratio );

		/* ---		  Muuttujat		--- */

static char			mode, hpf;
static r_float		tablen, refdeltap, ratio, portaexp, noportaexp;
static long			portalen, noportalen;
static r_float *	table;
static r_float		curr_rfreq = FP0;	/* tämänhetkinen taajuus */

		/* ---		   Funktiot		--- */

/*		long VCOinit( char oscmode, char hpfilt,
 *						unsigned short tlen, r_float *taddr, r_float tcycles,
 *						r_float pratio, r_float initrfreq, r_float portalength,
 *						r_float noportalength )
 *
 *	Alustaa oskillaattorin. oscmode on aaltomuodon valitsin (ks. 3MU.h)
 *	hpfilt on lippu ylipäästösuodattimelle.
 *	taddr on osoitin aaltomuototaulukkoon ja tlen on sen pituus
 *	(liukuluku)näytteinä tai nolla, jos halutaan moden indikoimaa aaltoa.
 *	tcycles (ei nolla) sisältää taulukon sisältämien jaksojen määrän.
 *	pratio on pulssisuhde sisäiselle kanttiaallolle (0,50) väliltä ]0, 1[.
 *	initrfreq on rfreq alussa ekassa nuotissa olevaa portamentoa varten.
 *	portalength on portamenton ja noportalength normaalin siirtymisen
 *	pituus näytteinä.
 *	Palauttaa virhekoodin.
 */

long VCOinit( char oscmode, char hpfilt,
				unsigned short tlen, r_float *taddr, r_float tcycles,
				r_float pratio, r_float initrfreq, r_float portalength,
				r_float noportalength )
{
	if( portalength == FP0 || noportalength == FP0 )
	{
		puts( "bug: Zero portamento length (vco.c)" );
		return( 1 );
	}
	mode = oscmode;
	hpf = hpfilt;
	ratio = pratio;
	curr_rfreq = initrfreq;
	portaexp = r_div( FP1, portalength );
	noportaexp = r_div( FP1, noportalength );
	portalen = r_trunc( portalength );
	noportalen = r_trunc( noportalength );

	if( mode == WAVE_SQUARE || mode == WAVE_SAWTOOTH )
	{
		table = NULL;
		tablen = FP1;	/* näin saadaan O303:lle vaihe välille [0, 1[ */
	}
	else if( mode == WAVE_ONE_SHOT || mode == WAVE_LOOPED )
	{
		if( !tlen )
		{
			puts( "bug: Zero table length (vco.c)" );
			return( 1 );
		}
		if( taddr == NULL )
		{
			puts( "bug: Missing wavetable (vco.c)" );
			return( 1 );
		}
		table = taddr;
		tablen = r_itof( tlen );
	} else if( mode != WAVE_INPUT )
	{
		puts( "bug: Unknown mode (vco.c)" );
		return( 1 );
	}
	if( tcycles == FP0 )
	{
		puts( "bug: Zero number of cycles (vco.c)" );
		return( 1 );
	}
	refdeltap = r_div( tablen, tcycles );
	return( 0 );
}

/*		r_float VCO( r_float rfreq, r_float env, BOOL porta, BOOL trig );
 *
 *	Tuottaa uuden rfreq-taajuisen samplen. env on vaippakäyrä
 *	porta on portamento-vipu
 *	rfreq on näytteenottotaajuuteen suhteellinen taajuus ]0, 0.5] (Nyquist)
 *	trig on trigger-tulo eli 1, jos pitää aloittaa
 *	uusi ääni (koskee lähinnä one-shot-modea).
 *	Palauttaa uuden samplen.
 */

r_float VCO( r_float rfreq, r_float env, BOOL porta, BOOL trig )
{
	/*	phase		tämänhetkinen aallon vaihe [0, tablen[
	 *	deltap		vaiheen muutos (taajuudesta riippuvainen)
	 *	shoot		jos 1, palautellaan table-näytteitä; jos 0, nollia
	 */
	static r_float phase = FP0, deltap, prev_rfreq = FP0;
	static r_float portacoeff = FP0;
	static long portacnt = 0;
	static BOOL shoot = 0;
	r_float out;

	/* trigataan uusi ääni */
	if( trig )
	{
		shoot = 1;
		if( mode == WAVE_ONE_SHOT )
			phase = FP0;
	}
	/* trigataan portamento */
	if( rfreq != prev_rfreq )
	{
		prev_rfreq = rfreq;
		/* vakiokestoinen portamento */
		portacnt = porta ? portalen : noportalen;
		portacoeff = r_pow( r_div( rfreq, curr_rfreq ),
						porta ? portaexp : noportaexp );
	}
	if( portacnt >= 0 )
	{
		if( portacnt )
			curr_rfreq = r_mul( curr_rfreq, portacoeff );
		else
			curr_rfreq = rfreq;

		rfreq = r_add( curr_rfreq, env );
		if( r_cmpable( rfreq ) > r_cmpable( FP0_5 ))
			rfreq = FP0_5;	/* Nyquist strikes again */
		deltap = r_mul( refdeltap, rfreq );
		portacnt--;
	}
	if( !shoot )
		return( FP0 );

	/* lasketaan uusi näyte */

	if( table == NULL )
		out = O303( phase, ratio );
	else
	{
		/* yksinkertainen lineaarinen interpolointi */
		unsigned short indf = r_trunc( phase );
		unsigned short indt = ( indf + 1 ) % r_trunc( tablen );
		r_float intf = table[ indf ];
		r_float intt = table[ indt ];
		r_float intr = r_sub( phase, r_itof( indf ));
		out = r_add( intf, r_mul( intr, r_sub( intt, intf )));
	}
	/* päivitetään oskillaattorin vaihe */

	phase = r_add( phase, deltap );
	if( r_cmpable( phase ) >= r_cmpable( tablen ))
	{
		if( mode == WAVE_ONE_SHOT )
			shoot = 0;
		else
			phase = r_sub( phase, tablen );
	}
	return( out );
}

/*		r_float O303( r_float rphase, r_float rratio )
 *
 *	303:n oskillaattoria matkiva koodinpätkä. mode valitsee joko kantti- tai
 *	saha-aallon, hpf laittaa päälle ylipäästösuotimen (303:n emuloimiseksi)
 *	rphase on vaihe; kuuluu alueelle [0, 1[, rratio on kanttiaallon
 *	pulssisuhde ]0, 1[.
 *	Palauttaa liukulukunäytteen.
 */

static r_float O303( r_float rphase, r_float rratio )
{
	static r_float curr_wave = FP0, ideal_wave = FP1;
	static char rise = -1;	/* lippu */
	static BOOL prev_half = 0;
	BOOL curr_half;
	r_float out;

	/* vakioita: */

#ifdef _DCC
#define	RISESPD		afp("0.95")
#define	FALLSPD		afp("0.9")
#define	HPFCOEFF	afp("0.99")
#else
#define	RISESPD		(0.95)
#define	FALLSPD		(0.9)
#define	HPFCOEFF	(0.99)
#endif
	const r_float risespd = RISESPD /* 0.2 */, fallspd = FALLSPD;
	const r_float hpf_coeff = HPFCOEFF;

	if( rise == -1 )
		rise = mode;	/* alustetaan rise */

	curr_half = r_cmpable( rphase ) >= r_cmpable( rratio ) ? 1 : 0;
	if( curr_half != prev_half )
	{
		prev_half = curr_half;
		rise = 1 - rise;
		/* square wave: */
		if( mode )
			ideal_wave = r_neg( ideal_wave );
	}
	if( !mode )
	/* sawtooth wave: */
		ideal_wave = r_sub( r_mul( rphase, FP2 ), FP1 );

/* Apply some inertia to the ideal waveform;
 * The squarewave rises faster than it falls, and that means that the
 * rising edge is sharper and will have more resonance. I guess it is
 * caused by the VCO being an integrator with a reset circuit for the
 * sawtooth wave, and the squarewave is simply a "hard limited" version
 * of the saw, fed through an op-amp comparator.
 * (lars@scala.no)
 */
	if( rise )
		curr_wave = r_add( curr_wave, r_mul( r_sub( ideal_wave, curr_wave ),
						risespd ));
	else
		curr_wave = r_add( curr_wave, r_mul( r_sub( ideal_wave, curr_wave ),
						fallspd ));

	out = curr_wave;

/* Highpass filter to get the "falling down to zero" look.
 * This is probably not what really happens, because the 303 has lots of
 * bass output - so I guess it is caused by the filter. It is just a hack
 * to make the waveform look more like the original. I need a 303 sample
 * without resonance!
 * (lars@scala.no)
 */
	if( hpf )
	{
		static r_float delta = FP0;
		delta = r_add( delta, curr_wave );
		delta = r_mul( delta, hpf_coeff );
		out = delta;
		delta = r_sub( delta, curr_wave );
	}

	return( out );
}

		/* ---		    Loppu		--- */
