/*-
 * Copyright © 2011, 2014, 2015, 2021, 2022, 2023
 *	mirabilos <m@mirbsd.org>
 *
 * Provided that these terms and disclaimer and all copyright notices
 * are retained or reproduced in an accompanying document, permission
 * is granted to deal in this work without restriction, including un‐
 * limited rights to use, publicly perform, distribute, sell, modify,
 * merge, give away, or sublicence.
 *
 * This work is provided “AS IS” and WITHOUT WARRANTY of any kind, to
 * the utmost extent permitted by applicable law, neither express nor
 * implied; without malicious intent or gross negligence. In no event
 * may a licensor, author or contributor be held liable for indirect,
 * direct, other damage, loss, or other issues arising in any way out
 * of dealing in the work, even if advised of the possibility of such
 * damage or existence of a defect, except proven that it results out
 * of said person’s immediate fault when using the work as intended.
 *-
 * This file provides BAFH1-0 (Better Avalanche for the Jenkins Hash)
 * as macro bodies operating on “register k32” variables (from sh.h),
 * which must be of u_int (or higher) rank, ≥32 bits width.
 *-
 * Little quote gem:
 *	We are looking into it. Changing the core
 *	hash function in PHP isn't a trivial change
 *	and will take us some time.
 *		-- Rasmus Lerdorf
 */

#ifndef MKSH_MIRHASH_H
#define MKSH_MIRHASH_H

__RCSID("$MirOS: src/bin/mksh/mirhash.h,v 1.20 2023/08/22 22:42:33 tg Exp $");

/*-
 * BAFH1-0 is defined by the following primitives:
 *
 * • BAFHInit(ctx) initialises the hash context, which consists of a
 *   sole 32-bit unsigned integer (ideally in a register), to 0^H1.
 *   It is possible to use any initial value out of [0; 2³²[ — which
 *   is, in fact, recommended if using BAFH for entropy distribution
 *   — but for a regular stable hash, the IV 0^H1 is needed: iff the
 *   sum of ctx and val in BAFHUpdateOctet is 0, leading NULs aren’t
 *   counted, so apply suitable bias to ensure that state is avoided.
 *
 * • BAFHUpdateOctet(ctx,val) compresses the unsigned 8-bit quantity
 *   into the hash context using Jenkins’ one-at-a-time algorithm.
 *
 * • BAFHFinish(ctx) avalanches the context around so every sub-byte
 *   depends on all input octets; afterwards, the context variable’s
 *   value is the hash output. BAFH does not use any padding, nor is
 *   the input length added; this is due to the common use case (for
 *   quick entropy distribution and use with a hashtable).
 *   Warning: BAFHFinish uses the MixColumn algorithm of AES — which
 *   is reversible (to avoid introducing funnels and reducing entro‐
 *   py), so blinding may need to be employed for some uses, e.g. in
 *   mksh, after a fork. For ctx == 0 only, ctx will be 0 afterwards.
 *
 * The following high-level macros are available:
 *
 * • BAFHUpdateMem(ctx,buf,len) adds a memory block to a context.
 * • BAFHUpdateStr(ctx,buf) is equivalent to using len=strlen(buf).
 * • BAFHUpdateVLQ(ctx,ut,ival) encodes as VLQ with negated A bit.
 *
 * All macros may use ctx multiple times in their expansion, but all
 * other arguments are always evaluated at most once.
 */

#define BAFHInit(h) do {					\
	(h) = mbiMM(k32, K32_FM, 1U);				\
} while (/* CONSTCOND */ 0)

#define BAFHUpdateOctet(h,b) do {				\
	(h) = mbiMO(k32, K32_FM, (h), +, KBI(b));		\
	(h) = mbiMO(k32, K32_FM, (h), +,			\
	    mbiMKshl(k32, K32_FM, (h), 10));			\
	(h) = mbiMO(k32, K32_FM, (h), ^,			\
	    mbiMKshr(k32, K32_FM, (h), 6));			\
} while (/* CONSTCOND */ 0)

#define BAFHFinish__impl(h,v,d)					\
	v = mbiMO(k32, K32_FM,					\
	    mbiMKshr(k32, K32_FM, h, 7), &, 0x01010101U);	\
	v = mbiMO(k32, K32_FM, v, +,				\
	    mbiMKshl(k32, K32_FM, v, 1));			\
	v = mbiMO(k32, K32_FM, v, +,				\
	    mbiMKshl(k32, K32_FM, v, 3));			\
	v = mbiMO(k32, K32_FM, v, ^, mbiMO(k32, K32_FM,		\
	    mbiMKshl(k32, K32_FM, h, 1), &, 0xFEFEFEFEU));	\
								\
	v = mbiMO(k32, K32_FM, v, ^,				\
	    mbiMKror(k32, K32_FM, v, 8));			\
	v = mbiMO(k32, K32_FM, v, ^,				\
	    (h = mbiMKror(k32, K32_FM, h, 8)));			\
	v = mbiMO(k32, K32_FM, v, ^,				\
	    (h = mbiMKror(k32, K32_FM, h, 8)));			\
	d = mbiMO(k32, K32_FM, v, ^,				\
	    mbiMKror(k32, K32_FM, h, 8));

#define BAFHFinish(h) do {					\
	register k32 BAFHFinish_v;				\
								\
	BAFHFinish__impl((h), BAFHFinish_v, (h))		\
} while (/* CONSTCOND */ 0)

#define BAFHUpdateMem(h,p,z) do {				\
	register const unsigned char *BAFHUpdate_p;		\
	register const unsigned char *BAFHUpdate_d;		\
								\
	BAFHUpdate_p = (const void *)(p);			\
	BAFHUpdate_d = BAFHUpdate_p + (z);			\
	while (BAFHUpdate_p < BAFHUpdate_d)			\
		BAFHUpdateOctet((h), *BAFHUpdate_p++);		\
} while (/* CONSTCOND */ 0)

#define BAFHUpdateStr(h,s) do {					\
	register const unsigned char *BAFHUpdate_s;		\
	register unsigned char BAFHUpdate_c;			\
								\
	BAFHUpdate_s = (const void *)(s);			\
	while ((BAFHUpdate_c = *BAFHUpdate_s++))		\
		BAFHUpdateOctet((h), BAFHUpdate_c);		\
} while (/* CONSTCOND */ 0)

#define BAFHUpdateVLQ(h,t,n) do {				\
	unsigned char BAFHUpdate_s[(sizeof(t)*(CHAR_BIT)+6)/7];	\
	register size_t BAFHUpdate_n = sizeof(BAFHUpdate_s);	\
	t BAFHUpdate_v = (n);					\
								\
	do {							\
		BAFHUpdate_s[--BAFHUpdate_n] =			\
		    BAFHUpdate_v & 0x7FU;			\
		BAFHUpdate_v >>= 7;				\
	} while (BAFHUpdate_v);					\
	BAFHUpdate_s[sizeof(BAFHUpdate_s) - 1] |= 0x80U;	\
	while (BAFHUpdate_n < sizeof(BAFHUpdate_s))		\
		BAFHUpdateOctet((h),				\
		    BAFHUpdate_s[BAFHUpdate_n++]);		\
} while (/* CONSTCOND */ 0)

#endif
