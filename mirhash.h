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
 * This file provides BAFH (Better Avalanche for the Jenkins Hash) as
 * inline macro bodies that operate on “register k32” variables (from
 * mksh/sh.h), with variants that use their local intermediate regs.
 *
 * Usage note for BAFH with entropy distribution: input up to 4 bytes
 * is best combined into a 32-bit unsigned integer, which is then run
 * through BAFHFinish_reg for mixing and then used as context instead
 * of 0. Longer input should be handled the same: take the first four
 * bytes as IV after mixing then add subsequent bytes the same way.
 * This needs counting input bytes and is endian-dependent, thus not,
 * for speed reasons, specified for the regular stable hash, but very
 * much recommended if the actual output value may differ across runs
 * (so is using a random value instead of 0 for the IV).
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

__RCSID("$MirOS: src/bin/mksh/mirhash.h,v 1.10 2022/01/06 22:34:59 tg Exp $");

/*-
 * BAFH itself is defined by the following primitives:
 *
 * • BAFHInit(ctx) initialises the hash context, which consists of a
 *   sole 32-bit unsigned integer (ideally in a register), to 0.
 *   It is possible to use any initial value out of [0; 2³²[ – which
 *   is, in fact, recommended if using BAFH for entropy distribution
 *   – but for a regular stable hash, the IV 0 is needed.
 *
 * • BAFHUpdateOctet(ctx,val) compresses the unsigned 8-bit quantity
 *   into the hash context. The algorithm used is Jenkins’ one-at-a-
 *   time, except that an additional constant 1 is added so that, if
 *   the context is (still) zero, adding a NUL byte is not ignored.
 *
 * • BAFHror(eax,cl) evaluates to the unsigned 32-bit integer “eax”,
 *   rotated right by “cl” ∈ [1; 31] (no casting, be careful!) where
 *   “eax” is K32()d and “cl” must be within range.
 *
 * • BAFHFinish(ctx) avalanches the context around so every sub-byte
 *   depends on all input octets; afterwards, the context variable’s
 *   value is the hash output. BAFH does not use any padding, nor is
 *   the input length added; this is due to the common use case (for
 *   quick entropy distribution and use with a hashtable).
 *   Warning: BAFHFinish uses the MixColumn algorithm of AES – which
 *   is reversible (to avoid introducing funnels and reducing entro‐
 *   py), so blinding may need to be employed for some uses, e.g. in
 *   mksh, after a fork.
 *
 * The BAFHUpdateOctet and BAFHFinish are available in two flavours:
 * suffixed with _reg (assumes the context is in a register) or _mem
 * (which doesn’t).
 *
 * The following high-level macros (with _reg and _mem variants) are
 * available:
 *
 * • BAFHUpdateMem(ctx,buf,len) adds a memory block to a context.
 * • BAFHUpdateStr(ctx,buf) is equivalent to using len=strlen(buf).
 *
 * All macros may use ctx multiple times in their expansion, but all
 * other arguments are always evaluated at most once except BAFHror.
 *
 * To stay portable, encode numbers using ULEB128, or better and for
 * sortability, VLQ (same unsigned but big endian), normal or Git’s.
 */

#define BAFHInit(h) do {					\
	(h) = 0;						\
} while (/* CONSTCOND */ 0)

#define BAFHUpdateOctet_reg(h,b) do {				\
	(h) = K32(K32(h) + KBI(b) + 1U);			\
	(h) = K32((h) + K32((h) << 10));			\
	(h) = K32((h) ^ K32((h) >> 6));				\
} while (/* CONSTCOND */ 0)

#define BAFHUpdateOctet_mem(m,b) do {				\
	register k32 BAFH_h = K32(m);				\
								\
	BAFHUpdateOctet_reg(BAFH_h, (b));			\
	(m) = BAFH_h;						\
} while (/* CONSTCOND */ 0)

#define BAFHror(eax,cl) K32(K32(K32(eax) >> (cl)) | K32(K32(eax) << (32 - (cl))))

#define BAFHFinish__impl(h,v,d)					\
	v = K32(K32(h) >> 7) & 0x01010101U;			\
	v = K32(v + K32(v << 1));				\
	v = K32(v + K32(v << 3));				\
	v = K32(v ^ (K32(K32(h) << 1) & 0xFEFEFEFEU));		\
								\
	v = K32(v ^ BAFHror(v, 8));				\
	v = K32(v ^ (h = BAFHror(h, 8)));			\
	v = K32(v ^ (h = BAFHror(h, 8)));			\
	d = K32(v ^ BAFHror(h, 8));

#define BAFHFinish_reg(h) do {					\
	register k32 BAFHFinish_v;				\
								\
	BAFHFinish__impl((h), BAFHFinish_v, (h))		\
} while (/* CONSTCOND */ 0)

#define BAFHFinish_mem(m) do {					\
	register k32 BAFHFinish_v, BAFH_h = (m);		\
								\
	BAFHFinish__impl(BAFH_h, BAFHFinish_v, (m))		\
} while (/* CONSTCOND */ 0)

#define BAFHUpdateMem_reg(h,p,z) do {				\
	register const unsigned char *BAFHUpdate_p;		\
	register size_t BAFHUpdate_z = (z);			\
								\
	BAFHUpdate_p = (const void *)(p);			\
	while (BAFHUpdate_z--)					\
		BAFHUpdateOctet_reg((h), *BAFHUpdate_p++);	\
} while (/* CONSTCOND */ 0)

/* meh should have named them _r/m but that’s not valid C */
#define BAFHUpdateMem_mem(m,p,z) do {				\
	register k32 BAFH_h = (m);				\
								\
	BAFHUpdateMem_reg(BAFH_h, (p), (z));			\
	(m) = BAFH_h;						\
} while (/* CONSTCOND */ 0)

#define BAFHUpdateStr_reg(h,s) do {				\
	register const unsigned char *BAFHUpdate_s;		\
	register unsigned char BAFHUpdate_c;			\
								\
	BAFHUpdate_s = (const void *)(s);			\
	while ((BAFHUpdate_c = *BAFHUpdate_s++) != 0)		\
		BAFHUpdateOctet_reg((h), BAFHUpdate_c);		\
} while (/* CONSTCOND */ 0)

#define BAFHUpdateStr_mem(m,s) do {				\
	register k32 BAFH_h = (m);				\
								\
	BAFHUpdateStr_reg(BAFH_h, (s));				\
	(m) = BAFH_h;						\
} while (/* CONSTCOND */ 0)

#endif
