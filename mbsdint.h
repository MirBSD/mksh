/*-
 * MirBSD sanitisation attempt for C integer madness
 *
 * Published under Ⓕ CC0
 */

#ifndef SYSKERN_MBSDINT_H
#define SYSKERN_MBSDINT_H "$MirOS: src/bin/mksh/mbsdint.h,v 1.2 2022/01/28 03:49:12 tg Exp $"

/* if you have <sys/types.h> and/or <stdint.h>, include them before this */

#if !defined(_KERNEL) && !defined(_STANDALONE)
#include <limits.h>
#include <stddef.h>
#else
#include <sys/cdefs.h>
#include <sys/limits.h>
#include <sys/stddef.h>
#endif

/* should be in <sys/cdefs.h> via <limits.h> */
#ifndef __predict_true
#if defined(__GNUC__) && (__GNUC__ >= 3) /* 2.96, but keep it simple here */
#define __predict_true(exp)	__builtin_expect(!!(exp), 1)
#define __predict_false(exp)	__builtin_expect(!!(exp), 0)
#else
#define __predict_true(exp)	(!!(exp))
#define __predict_false(exp)	(!!(exp))
#endif /* !GCC 3.x */
#endif /* ndef(__predict_true) */

#if !defined(SIZE_MAX) && defined(SIZE_T_MAX)
#define SIZE_MAX		SIZE_T_MAX
#endif

/* compile-time assertions: mbiCTAS(srcf_c) { … }; */
#define mbiCTAS(name)		struct ctassert_ ## name
#define mbiCTA(name,cond)	char cta_ ## name [(cond) ? 1 : -1]
/* special CTAs */
#define mbiCTA_TYPE_NOTF(type)	char ctati_ ## type [((type)0.5 == 0) ? 1 : -1]

/* largest integer */
#if defined(INTMAX_MIN)
#define mbiHUGE_S		intmax_t
#define mbiHUGE_MIN		INTMAX_MIN
#define mbiHUGE_MAX		INTMAX_MAX
#define mbiHUGE_U		uintmax_t
#define mbiHUGE_UMAX		UINTMAX_MAX
#elif defined(LLONG_MIN)
#define mbiHUGE_S		long long
#define mbiHUGE_MIN		LLONG_MIN
#define mbiHUGE_MAX		LLONG_MAX
#define mbiHUGE_U		unsigned long long
#define mbiHUGE_UMAX		ULLONG_MAX
#else
#define mbiHUGE_S		long
#define mbiHUGE_MIN		LONG_MIN
#define mbiHUGE_MAX		LONG_MAX
#define mbiHUGE_U		unsigned long
#define mbiHUGE_UMAX		ULONG_MAX
#endif

/* kinds of types */
	/* runtime only, but see mbiCTA_TYPE_NOTF */
#define mbiTYPE_ISF(type)	((double)0.5 == (double)(type)0.5)
	/* compile-time and runtime */
#define mbiTYPE_ISU(type)	((type)-1 > 0)
/* limits of types */
#define mbiTYPE_UMAX(type)	((type)~(type)0)
#define mbiTYPE_UBITS(type)	mbiMASK_BITS(mbiTYPE_UMAX(type))
/* calculation by Hallvard B Furuseth (from comp.lang.c), up to 2039 bits */
#define mbiMASK_BITS(maxv)	((unsigned int)((maxv) / ((maxv) % 255 + 1) / \
				255 % 255 * 8 + 7 - 86 / ((maxv) % 255 + 12)))

/* ensure v is a positive (2ⁿ-1) number (n>0), up to 279 bits */
#define mbiMASK_CHK(v)		((v) > 0 ? mbi_maskchk31_1((v)) : 0)
#define mbi_maskchks(v,m,o,n)	(v <= m ? o : ((v & m) == m) && n)
#define mbi_maskchk31s(v,n)	mbi_maskchks(v, 0x7FFFFFFFUL, \
				    mbi_maskchk16(v), n)
#define mbi_maskchk31_1(v)	mbi_maskchk31s(v, mbi_maskchk31_2(v >> 31))
#define mbi_maskchk31_2(v)	mbi_maskchk31s(v, mbi_maskchk31_3(v >> 31))
#define mbi_maskchk31_3(v)	mbi_maskchk31s(v, mbi_maskchk31_4(v >> 31))
#define mbi_maskchk31_4(v)	mbi_maskchk31s(v, mbi_maskchk31_5(v >> 31))
#define mbi_maskchk31_5(v)	mbi_maskchk31s(v, mbi_maskchk31_6(v >> 31))
#define mbi_maskchk31_6(v)	mbi_maskchk31s(v, mbi_maskchk31_7(v >> 31))
#define mbi_maskchk31_7(v)	mbi_maskchk31s(v, mbi_maskchk31_8(v >> 31))
#define mbi_maskchk31_8(v)	mbi_maskchk31s(v, mbi_maskchk31_9(v >> 31))
#define mbi_maskchk31_9(v)	(v <= 0x7FFFFFFFUL && mbi_maskchk16(v))
#define mbi_maskchk16(v)	mbi_maskchks(v, 0xFFFFU, \
				    mbi_maskchk8(v), mbi_maskchk8(v >> 16))
#define mbi_maskchk8(v)		mbi_maskchks(v, 0xFFU, \
				    mbi_maskchk4(v), mbi_maskchk4(v >> 8))
#define mbi_maskchk4(v)		mbi_maskchks(v, 0xFU, \
				    mbi_maskchkF(v), mbi_maskchkF(v >> 4))
#define mbi_maskchkF(v)		(v == 0xF || v == 7 || v == 3 || v == 1 || !v)

/* compile-time assertions for mbsdint.h */
mbiCTAS(mbsdint_h) {
 /* compiler (in)sanity, from autoconf */
#define mbiCTf(x) 'x'
 mbiCTA(xlc6, mbiCTf(a) == 'x');
#undef mbiCTf
 mbiCTA(osf4, '\x00' == 0);
 /* C types */
 mbiCTA(basic_char,
	sizeof(char) == 1 && (CHAR_BIT) >= 7 && (CHAR_BIT) < 2040 &&
	mbiTYPE_UMAX(unsigned char) == (UCHAR_MAX) &&
	sizeof(signed char) == 1 &&
	sizeof(unsigned char) == 1 &&
	mbiMASK_CHK(SCHAR_MAX) && mbiMASK_CHK(UCHAR_MAX) &&
	((SCHAR_MIN) == -(SCHAR_MAX) || (SCHAR_MIN) == -(SCHAR_MAX)-1) &&
	mbiTYPE_UBITS(unsigned char) == (unsigned int)(CHAR_BIT));
 mbiCTA(basic_short,
	mbiTYPE_UMAX(unsigned short) == (USHRT_MAX) &&
	sizeof(short) <= (279 / CHAR_BIT) &&
	sizeof(unsigned short) <= (279 / CHAR_BIT) &&
	mbiMASK_CHK(SHRT_MAX) && mbiMASK_CHK(USHRT_MAX) &&
	((SHRT_MIN) == -(SHRT_MAX) || (SHRT_MIN) == -(SHRT_MAX)-1) &&
	sizeof(short) >= sizeof(signed char) &&
	sizeof(unsigned short) >= sizeof(unsigned char) &&
	sizeof(short) == sizeof(unsigned short));
 mbiCTA(basic_int,
	mbiTYPE_UMAX(unsigned int) == (UINT_MAX) &&
	sizeof(int) <= (279 / CHAR_BIT) &&
	sizeof(unsigned int) <= (279 / CHAR_BIT) &&
	mbiMASK_CHK(INT_MAX) && mbiMASK_CHK(UINT_MAX) &&
	((INT_MIN) == -(INT_MAX) || (INT_MIN) == -(INT_MAX)-1) &&
	sizeof(int) >= sizeof(short) &&
	sizeof(unsigned int) >= sizeof(unsigned short) &&
	sizeof(int) == sizeof(unsigned int));
 mbiCTA(basic_long,
	mbiTYPE_UMAX(unsigned long) == (ULONG_MAX) &&
	sizeof(long) <= (279 / CHAR_BIT) &&
	sizeof(unsigned long) <= (279 / CHAR_BIT) &&
	mbiMASK_CHK(LONG_MAX) && mbiMASK_CHK(ULONG_MAX) &&
	((LONG_MIN) == -(LONG_MAX) || (LONG_MIN) == -(LONG_MAX)-1) &&
	sizeof(long) >= sizeof(int) &&
	sizeof(unsigned long) >= sizeof(unsigned int) &&
	sizeof(long) == sizeof(unsigned long));
#ifdef LLONG_MIN
 mbiCTA(basic_quad,
	mbiTYPE_UMAX(unsigned long long) == (ULLONG_MAX) &&
	sizeof(long long) <= (279 / CHAR_BIT) &&
	sizeof(unsigned long long) <= (279 / CHAR_BIT) &&
	mbiMASK_CHK(LLONG_MAX) && mbiMASK_CHK(ULLONG_MAX) &&
	((LLONG_MIN) == -(LLONG_MAX) || (LLONG_MIN) == -(LLONG_MAX)-1) &&
	sizeof(long long) >= sizeof(long) &&
	sizeof(unsigned long long) >= sizeof(unsigned long) &&
	sizeof(long long) == sizeof(unsigned long long));
#endif
#ifdef INTMAX_MIN
 mbiCTA(basic_imax,
	mbiTYPE_UMAX(uintmax_t) == (UINTMAX_MAX) &&
	sizeof(intmax_t) <= (279 / CHAR_BIT) &&
	sizeof(uintmax_t) <= (279 / CHAR_BIT) &&
	mbiMASK_CHK(INTMAX_MAX) && mbiMASK_CHK(UINTMAX_MAX) &&
	((INTMAX_MIN) == -(INTMAX_MAX) || (INTMAX_MIN) == -(INTMAX_MAX)-1) &&
#ifdef LLONG_MIN
	sizeof(intmax_t) >= sizeof(long long) &&
	sizeof(uintmax_t) >= sizeof(unsigned long long) &&
#else
	sizeof(intmax_t) >= sizeof(long) &&
	sizeof(uintmax_t) >= sizeof(unsigned long) &&
#endif
	sizeof(intmax_t) == sizeof(uintmax_t));
#endif
 /* size_t is C but ssize_t is POSIX */
 mbiCTA_TYPE_NOTF(size_t);
 mbiCTA(basic_sizet,
	sizeof(size_t) >= sizeof(unsigned int) &&
	sizeof(size_t) <= sizeof(mbiHUGE_U) &&
	mbiTYPE_ISU(size_t) &&
#ifdef SIZE_MAX
	mbiMASK_CHK(SIZE_MAX) &&
	mbiMASK_BITS(SIZE_MAX) <= mbiTYPE_UBITS(size_t) &&
	((SIZE_MAX) == (mbiHUGE_U)(size_t)(SIZE_MAX)) &&
#endif
	mbiTYPE_UBITS(size_t) <= mbiMASK_BITS(mbiHUGE_UMAX));
 mbiCTA_TYPE_NOTF(ptrdiff_t);
 mbiCTA(basic_ptrdifft,
	sizeof(ptrdiff_t) >= sizeof(int) &&
	sizeof(ptrdiff_t) <= sizeof(mbiHUGE_S) &&
	!mbiTYPE_ISU(ptrdiff_t) &&
#ifdef PTRDIFF_MAX
	mbiMASK_CHK(PTRDIFF_MAX) &&
	((PTRDIFF_MAX) == (mbiHUGE_S)(ptrdiff_t)(PTRDIFF_MAX)) &&
#endif
	1);
 /* UGH! off_t is POSIX, with no _MIN/_MAX constants… WTF‽ */
#ifdef SSIZE_MAX
 mbiCTA_TYPE_NOTF(ssize_t);
 mbiCTA(basic_ssizet,
	sizeof(ssize_t) == sizeof(size_t) &&
	!mbiTYPE_ISU(ssize_t) &&
	mbiMASK_CHK(SSIZE_MAX) &&
	((SSIZE_MAX) == (mbiHUGE_S)(ssize_t)(SSIZE_MAX)) &&
#ifdef SIZE_MAX
	mbiMASK_BITS(SSIZE_MAX) <= mbiMASK_BITS(SIZE_MAX) &&
#endif
	mbiMASK_BITS(SSIZE_MAX) < mbiTYPE_UBITS(size_t));
#endif
 /* C99 §6.2.6.2(1, 2, 6) permits M ≤ N, but M < N is normally desired */
 /* here require signed/unsigned types to have same width (M=N-1) */
 mbiCTA(vbits_char, mbiMASK_BITS(UCHAR_MAX) == mbiMASK_BITS(SCHAR_MAX) + 1U);
 mbiCTA(vbits_short, mbiMASK_BITS(USHRT_MAX) == mbiMASK_BITS(SHRT_MAX) + 1U);
 mbiCTA(vbits_int, mbiMASK_BITS(UINT_MAX) == mbiMASK_BITS(INT_MAX) + 1U);
 mbiCTA(vbits_long, mbiMASK_BITS(ULONG_MAX) == mbiMASK_BITS(LONG_MAX) + 1U);
#ifdef LLONG_MIN
 mbiCTA(vbits_quad, mbiMASK_BITS(ULLONG_MAX) == mbiMASK_BITS(LLONG_MAX) + 1U);
#endif
#ifdef INTMAX_MIN
 mbiCTA(vbits_imax, mbiMASK_BITS(UINTMAX_MAX) == mbiMASK_BITS(INTMAX_MAX) + 1U);
#endif
 /* require pointers and size_t to take up the same amount of space */
 mbiCTA(sizet_voidptr, sizeof(size_t) == sizeof(void *));
 mbiCTA(sizet_funcptr, sizeof(size_t) == sizeof(void (*)(void)));
#if 0 /* breaks on LLP64 (e.g. Windows/amd64) */
 mbiCTA(sizet_inulong, sizeof(size_t) <= sizeof(long));
#endif
 /* assume ptrdiff_t and (s)size_t will fit each other */
 mbiCTA(sizet_ptrdiff, sizeof(size_t) == sizeof(ptrdiff_t));
};

/*
 * calculations where the unsigned type is used as backing store,
 * doing the two’s complement conversion manually (to avoid UB/IB);
 * (M=N-1), see above, is required for these to work (symmetrically)
 *
 * ut = unsigned type (e.g. unsigned long); st = signed type (e.g. long)
 * SM = signed max (e.g. LONG_MAX); v = value
 * m = magnitude (≥ 0 value); vz = signbit (0:value; 1:-value)
 * HM = half mask (e.g. (ut)0x7FFFUL); FM = full mask (e.g. (ut)0xFFFFUL)
 */

/* 1. casting between unsigned(two’s complement) and signed(native) */
#define mbiA_U2S(ut,st,SM,v)	(((v) > (ut)(SM)) ? \
				(st)(-(st)(~(v)) - (st)1) : \
				(st)(v))
#define mbiA_S2U(ut,st,v)	(((v) < 0) ? \
				(ut)(~(ut)(-((v) + (st)1))) : \
				(ut)(v))

/* 2. converting between signed(native) and signbit(vz) plus magnitude */
#define mbiA_VZM2S(ut,st,vz,m)	(((vz) && (m) > 0) ? \
				(st)(-(st)((m) - (ut)1) - (st)1) : \
				(st)(m))
#define mbiA_S2VZ(v)		((v) < 0)
#define mbiA_S2M(ut,st,v)	(((v) < 0) ? \
				(ut)((ut)(-((v) + (st)1)) + (ut)1) : \
				(ut)(v))

/* 3. masking arithmetics in possibly-longer type */
#define mbiMA_U2S(ut,st,FM,HM,v) \
				(((ut)((v) & FM) > HM) ? \
				(st)(-(st)(~(v) & HM) - (st)1) : \
				(st)((v) & HM))
#define mbiMA_S2U(ut,st,FM,v)	((ut)(mbiA_S2U(ut, st, v) & FM))
#define mbiMA_VZM2S(ut,st,FM,HM,vz,m) \
				(((vz) && (ut)((m) & FM) > 0) ? \
				(st)(-(st)(((m) - (ut)1) & HM) - (st)1) : \
				(st)((m) & HM))
#define mbiMA_S2VZ(v)		((v) < 0)
#define mbiMA_S2M(ut,st,HM,v)	(((v) < 0) ? \
				(ut)((ut)((-((v) + (st)1)) & HM) + (ut)1) : \
				(ut)((ut)(v) & HM))

/*
 * UB-safe overflow/underflow-checking integer arithmetics
 *
 * Before using, #define mbiCfail to the action that should be
 * taken on check (e.g. "goto fail" or "return (0)"); make sure
 * to #undef mbiCfail before the end of the function body though!
 *
 * There is IB on some signed operations, but that is only either
 * the result (which is checked) is implementation-defined, thus
 * safe, or an implementation-defined signal is raised, which is
 * the caller’s problem ☻
 */

/* 1. for unsigned types checking afterwards is OK */
#define mbiCAUinc(vl)		mbiCAUadd(vl, 1)
#define mbiCAUdec(vl)		mbiCAUsub(vl, 1)
#define mbiCAUadd(vl,vr)	do {					\
	(vl) += (vr);							\
	if (__predict_false((vl) < (vr)))				\
		mbiCfail;						\
} while (/* CONSTCOND */ 0)
#define mbiCAUsub(vl,vr)	do {					\
	if (__predict_false((vl) < (vr)))				\
		mbiCfail;						\
	(vl) -= (vr);							\
} while (/* CONSTCOND */ 0)
#define mbiCAUmul(vl,vr)	do {					\
	if (__predict_false((((vl) * (vr)) / (vr)) < (vl)))		\
		mbiCfail;						\
	(vl) *= (vr);							\
} while (/* CONSTCOND */ 0)

/* 2. for signed types; lim is a prefix (e.g. SHRT) */
#define mbiCASinc(lim,vl)	do {					\
	if (__predict_false((vl) == lim ## _MAX))			\
		mbiCfail;						\
	++(vl);								\
} while (/* CONSTCOND */ 0)
#define mbiCASdec(lim,vl)	do {					\
	if (__predict_false((vl) == lim ## _MIN))			\
		mbiCfail;						\
	--(vl);								\
} while (/* CONSTCOND */ 0)
#define mbiCASadd(lim,vl,vr)	do {					\
	if (__predict_false((vl) > (lim ## _MAX - (vr))))		\
		mbiCfail;						\
	(vl) += (vr);							\
} while (/* CONSTCOND */ 0)
#define mbiCASsub(lim,vl,vr)	do {					\
	if (__predict_false((vl) < (lim ## _MIN + (vr))))		\
		mbiCfail;						\
	(vl) -= (vr);							\
} while (/* CONSTCOND */ 0)
#define mbiCASmul(lim,vl,vr)	do {					\
	if (__predict_false((lim ## _MAX / (vr)) < (vl) ||		\
	    (lim ## _MIN / (vr)) > (vl)))				\
		mbiCfail;						\
	(vl) *= (vr);							\
} while (/* CONSTCOND */ 0)

/* 3. signed narrowing assign; this is IB */
#define mbiCASlet(dsttype,vl,srctype,vr) do {				\
	(vl) = (dsttype)(vr);						\
	if (__predict_false((srctype)(vl) != (vr)))			\
		mbiCfail;						\
} while (/* CONSTCOND */ 0)

/*
 * calculations on manual two’s complement
 *
 * addition, subtraction and multiplication, bitwise and boolean
 * operations, as well as equality comparisons can be done on the
 * unsigned value; other comparisons must be done on the signed
 * value and the other operations follow
 */

/* rotate and shift: pass *unsigned* values; shr shifts in vz bits */
#define mbiVAU_shift(ut,dst,vl,vr,rv) do {				\
	(dst) = (vr) & (mbiTYPE_UBITS(ut) - 1);				\
	(dst) = (dst) ? (rv) : (vl);					\
} while (/* CONSTCOND */ 0)
#define mbiVAUrol(ut,dst,vl,vr) mbiVAU_shift(ut, dst, vl, vr, \
	((vl) << (dst)) | ((vl) >> (mbiTYPE_UBITS(ut) - (dst))))
#define mbiVAUror(ut,dst,vl,vr) mbiVAU_shift(ut, dst, vl, vr, \
	((vl) >> (dst)) | ((vl) << (mbiTYPE_UBITS(ut) - (dst))))
#define mbiVAUshl(ut,dst,vl,vr) mbiVAU_shift(ut, dst, vl, vr, \
	(vl) << (dst))
#define mbiVAUshr(ut,dst,vz,vl,vr) mbiVAU_shift(ut, dst, vl, vr, \
	(vz) ? (ut)~((ut)~(vl) >> (dst)) : (vl) >> (dst))
#define mbiM_do(dst,FM,act) do { act; (dst) &= FM; } while (/* CONSTCOND */ 0)
#define mbiMVAUrol(ut,FM,dst,vl,vr) mbiM_do(dst, FM, mbiVAUrol(ut,dst,vl,vr))
#define mbiMVAUror(ut,FM,dst,vl,vr) mbiM_do(dst, FM, mbiVAUror(ut,dst,vl,vr))
#define mbiMVAUshl(ut,FM,dst,vl,vr) mbiM_do(dst, FM, mbiVAUshl(ut,dst,vl,vr))
#define mbiMVAUshr(ut,FM,dst,vz,vl,vr) \
	mbiM_do(dst, FM, mbiVAUshr(ut,dst,vz,vl,vr))

/* division and remainder; pass *signed* values and unsigned result vars */
#define mbiVASdivrem(ut,st,udiv,urem,vl,vr) do {			\
	(udiv) = mbiA_S2M(ut, st, (vl)) / mbiA_S2M(ut, st, (vr));	\
	if (mbiA_S2VZ(vl) ^ mbiA_S2VZ(vr))				\
		(udiv) = -(udiv);					\
	(urem) = mbiA_S2U(ut, st, (vl)) -				\
	    ((udiv) * mbiA_S2U(ut, st, (vr)));				\
} while (/* CONSTCOND */ 0)
#define mbiMVASdivrem(ut,st,FM,HM,udiv,urem,vl,vr) do {			\
	(udiv) = mbiMA_S2M(ut, st, HM, (vl)) /				\
	    mbiMA_S2M(ut, st, HM, (vr));				\
	if (mbiMA_S2VZ(vl) ^ mbiMA_S2VZ(vr))				\
		(udiv) = -(udiv);					\
	(urem) = mbiA_S2U(ut, st, (vl)) -				\
	    ((udiv) * mbiA_S2U(ut, st, (vr)));				\
	(udiv) &= FM;							\
	(urem) &= FM;							\
} while (/* CONSTCOND */ 0)

#endif /* !SYSKERN_MBSDINT_H */
