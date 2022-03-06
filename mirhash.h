/*-
 * Copyright © 2011, 2014, 2015, 2021, 2022
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
 * which must be of u_int (or higher) rank.
 *-
 * Little quote gem:
 *	We are looking into it. Changing the core
 *	hash function in PHP isn't a trivial change
 *	and will take us some time.
 *		-- Rasmus Lerdorf
 */

#ifndef MKSH_MIRHASH_H
#define MKSH_MIRHASH_H

#include <sys/types.h>

__RCSID("$MirOS: src/bin/mksh/mirhash.h,v 1.16 2022/03/06 01:52:57 tg Exp $");

/*-
 * BAFH1-0 is defined by the following primitives:
 *
 * • BAFHInit(ctx) initialises the hash context, which consists of a
 *   sole 32-bit unsigned integer (ideally in a register), to 0^H1.
 *   It is possible to use any initial value out of [0; 2³²[ — which
 *   is, in fact, recommended if using BAFH for entropy distribution
 *   — but for a regular stable hash, the IV 0^H1 is needed.
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
 *   mksh, after a fork.
 *
 * The following high-level macros are available:
 *
 * • BAFHUpdateMem(ctx,buf,len) adds a memory block to a context.
 * • BAFHUpdateStr(ctx,buf) is equivalent to using len=strlen(buf).
 *
 * All macros may use ctx multiple times in their expansion, but all
 * other arguments are always evaluated at most once.
 */

#define BAFHInit(h) do {					\
	(h) = K32(1U);						\
} while (/* CONSTCOND */ 0)

#define BAFHUpdateOctet(h,b) do {				\
	(h) = K32(KUI(h) + KBI(b));				\
	(h) = K32((h) + K32((h) << 10));			\
	(h) = K32((h) ^ K32((h) >> 6));				\
} while (/* CONSTCOND */ 0)

#define BAFHFinish(h) BAFHFinish__impl((h), BAFHFinish_v)
#define BAFHFinish__impl(h,v) do {				\
	register k32 v;						\
								\
	v = K32(K32(h) >> 7) & 0x01010101U;			\
	v = K32(v + K32(v << 1));				\
	v = K32(v + K32(v << 3));				\
	v = K32(v ^ (K32(K32(h) << 1) & 0xFEFEFEFEU));		\
								\
	v = K32(v ^ mbiMKror(k32, K32_FM, v, 8));		\
	v = K32(v ^ (h = mbiMKror(k32, K32_FM, h, 8)));		\
	v = K32(v ^ (h = mbiMKror(k32, K32_FM, h, 8)));		\
	h = K32(v ^ mbiMKror(k32, K32_FM, h, 8));		\
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
	while ((BAFHUpdate_c = *BAFHUpdate_s++) != '\0')	\
		BAFHUpdateOctet((h), BAFHUpdate_c);		\
} while (/* CONSTCOND */ 0)

#endif
