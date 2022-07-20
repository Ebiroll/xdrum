/*
 *	convaud.h		Äänen eri esitysmuotojen väliset muunnokset
 *
 *	Copyright © 1996 Jarno Seppänen; see 3MU.c for licensing info.
 */

#ifndef CONVAUD_H
#define CONVAUD_H

#include "r_ffp/r_ffp.h"

#ifdef _DCC
#define FP32768	0x80000050		/* (2^16)/2	= 32768	*/
#define FP128	0x80000048		/* (2^8)/2	= 128	*/
#define FP20	0xA0000045
#else
#define FP32768	(32768.0)		/* jostain syystä 32768f ei kelpaa! */
#define FP128	(128.0)
#define	FP20	(20.0)
#endif

/* liukuluvun [-1.0, 1.0] muunnos kokonaisluvuksi välille [-r, r]
 */
#define ffp2int(f,r) r_trunc(r_mul((r_float)(f),(r_float)(r)))

/* liukulukujen [-1.0, 1.0] muunnos 16-bittisiksi eli välille [-32768, 32768[
 * ja päinvastoin
 */
#define ffp2aud16(f) ((short)((ffp2int((f),FP32768)==0x8000L)?0x7FFF:(ffp2int((f),FP32768))))
#define aud2ffp16(i) (r_div(r_itof(i),FP32768))

#if 0
#define ffp2aud16(f) ((short)((r_trunc(r_mul(FP65535,(r_float)(f)))-1)>>1))
#define aud2ffp16(i) (r_div(r_itof(((long)(i)<<1)+1),FP65535))
#endif

/* liukulukujen [-1.0, 1.0] muunnos 8-bittisiksi eli välille [-128, 128[
 * ja päinvastoin
 */
#define ffp2aud8(f) ((signed char)((ffp2int((f),FP128)==0x80L)?0x7F:(ffp2int((f),FP128))))
#define aud2ffp8(i) (r_div(r_itof(i),FP128))

#if 0
#define ffp2aud8(f)	((signed char)((r_trunc(r_mul(FP255,(r_float)(f)))-1)>>1))
#define aud2ffp8(i)	(r_div(r_itof(((long)(i)<<1)+1),FP255))
#endif

/* desibelien muunnos jännitteeksi ]0, inf] ja päinvastoin:	0 dB <-> 1
 */

#define	dB2ffp(d)	(r_pow(FP10,r_div((r_float)(d),FP20)))
#define	ffp2dB(f)	(r_mul(FP20,r_log10((r_float)(f))))

#endif
