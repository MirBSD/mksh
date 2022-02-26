/*-
 * MirBSD sanitisation attempt for C integer madness
 *
 * Published under Ⓕ CC0
 */

#ifndef SYSKERN_MBSDINT_H
#define SYSKERN_MBSDINT_H "$MirOS: src/bin/mksh/mbsdint.h,v 1.7 2022/02/26 05:38:05 tg Exp $"

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
	((SCHAR_MIN) == -(SCHAR_MAX) || (SCHAR_MIN)+1 == -(SCHAR_MAX)) &&
	mbiTYPE_UBITS(unsigned char) == (unsigned int)(CHAR_BIT));
 mbiCTA(basic_short,
	mbiTYPE_UMAX(unsigned short) == (USHRT_MAX) &&
	sizeof(short) <= (279 / CHAR_BIT) &&
	sizeof(unsigned short) <= (279 / CHAR_BIT) &&
	mbiMASK_CHK(SHRT_MAX) && mbiMASK_CHK(USHRT_MAX) &&
	((SHRT_MIN) == -(SHRT_MAX) || (SHRT_MIN)+1 == -(SHRT_MAX)) &&
	sizeof(short) >= sizeof(signed char) &&
	sizeof(unsigned short) >= sizeof(unsigned char) &&
	sizeof(short) == sizeof(unsigned short));
 mbiCTA(basic_int,
	mbiTYPE_UMAX(unsigned int) == (UINT_MAX) &&
	sizeof(int) <= (279 / CHAR_BIT) &&
	sizeof(unsigned int) <= (279 / CHAR_BIT) &&
	mbiMASK_CHK(INT_MAX) && mbiMASK_CHK(UINT_MAX) &&
	((INT_MIN) == -(INT_MAX) || (INT_MIN)+1 == -(INT_MAX)) &&
	sizeof(int) >= sizeof(short) &&
	sizeof(unsigned int) >= sizeof(unsigned short) &&
	sizeof(int) == sizeof(unsigned int));
 mbiCTA(basic_long,
	mbiTYPE_UMAX(unsigned long) == (ULONG_MAX) &&
	sizeof(long) <= (279 / CHAR_BIT) &&
	sizeof(unsigned long) <= (279 / CHAR_BIT) &&
	mbiMASK_CHK(LONG_MAX) && mbiMASK_CHK(ULONG_MAX) &&
	((LONG_MIN) == -(LONG_MAX) || (LONG_MIN)+1 == -(LONG_MAX)) &&
	sizeof(long) >= sizeof(int) &&
	sizeof(unsigned long) >= sizeof(unsigned int) &&
	sizeof(long) == sizeof(unsigned long));
#ifdef LLONG_MIN
 mbiCTA(basic_quad,
	mbiTYPE_UMAX(unsigned long long) == (ULLONG_MAX) &&
	sizeof(long long) <= (279 / CHAR_BIT) &&
	sizeof(unsigned long long) <= (279 / CHAR_BIT) &&
	mbiMASK_CHK(LLONG_MAX) && mbiMASK_CHK(ULLONG_MAX) &&
	((LLONG_MIN) == -(LLONG_MAX) || (LLONG_MIN)+1 == -(LLONG_MAX)) &&
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
	((INTMAX_MIN) == -(INTMAX_MAX) || (INTMAX_MIN)+1 == -(INTMAX_MAX)) &&
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
 mbiCTA(sizet_sintptr, sizeof(size_t) == sizeof(int *));
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
 *
 * Note: most of these will UB at the most negative value (0x80̅) if the
 * signed type implements sign-and-magnitude or one’s complement…
 */

/* 1. casting between unsigned(two’s complement) and signed(native) */
#define mbiA_U2S(ut,st,SM,v)	(mbiA_U2VZ(ut, (SM), (v)) ? \
				(st)(-(st)(~(v)) - (st)1) : \
				(st)(v))
#define mbiA_S2U(ut,st,v)	(mbiA_S2VZ(v) ? \
				(ut)(~(ut)(-((v) + (st)1))) : \
				(ut)(v))

/* 2. converting between signed(native) and signbit(vz) plus magnitude */
#define mbiA_VZM2S(ut,st,vz,m)	(((vz) && (m) > 0) ? \
				(st)(-(st)((m) - (ut)1) - (st)1) : \
				(st)(m))
#define mbiA_S2VZ(v)		((v) < 0)
#define mbiA_S2M(ut,st,v)	(mbiA_S2VZ(v) ? \
				(ut)((ut)(-((v) + (st)1)) + (ut)1) : \
				(ut)(v))
/* unsigned(two’s complement) and signbit(vz) plus magnitude */
#define mbiA_VZM2U(ut,SM,vz,m)	(((vz) && (m) > 0) ? \
				(ut)(~(ut)(((m) - (ut)1) & (ut)(SM))) : \
				(ut)((m) & (ut)(SM)))
/* magnitude cut off ↑ vs wrapping around ↓ */
#define mbiA_VZU2U(ut,vz,m)	((vz) ? \
				(ut)-((ut)(m)) : (ut)(m))
#define mbiA_U2VZ(ut,SM,v)	((v) > (ut)(SM))
#define mbiA_U2M(ut,SM,v)	(mbiA_U2VZ(ut, (SM), (v)) ? \
				(ut)-((ut)(v)) : (ut)(v))
/* note: the above 4 are nōn-2s-complement-safe */

/* 3. masking arithmetics in possibly-longer type */
#define mbiMM(ut,FM,v)		((ut)((ut)(v) & (ut)(FM)))
#define mbiMA_U2S(ut,st,FM,HM,v) \
				(mbiMA_U2VZ(ut, (FM), (HM), (v)) ? \
				(st)(-(st)(~(v) & (ut)(HM)) - (st)1) : \
				(st)((v) & (ut)(HM)))
#define mbiMA_S2U(ut,st,FM,v)	mbiMM(ut, (FM), mbiA_S2U(ut, st, (v)))
#define mbiMA_VZM2S(ut,st,FM,HM,vz,m) \
				(((vz) && mbiMM(ut, (FM), (m)) > 0) ? \
				(st)(-(st)(((m) - (ut)1) & (ut)(HM)) - (st)1) : \
				(st)((m) & (ut)(HM)))
#define mbiMA_S2VZ(v)		((v) < 0)
#define mbiMA_S2M(ut,st,HM,v)	(mbiMA_S2VZ(v) ? \
				(ut)((ut)((ut)(-((v) + (st)1)) & (ut)(HM)) + (ut)1) : \
				(ut)((ut)(v) & (ut)(HM)))
#define mbiMA_VZM2U(ut,FM,HM,vz,m) \
				(((vz) && mbiMM(ut, (FM), (m)) > 0) ? \
				mbiMM(ut, (FM), ~(ut)(((m) - (ut)1) & (ut)(HM))) : \
				(ut)((m) & (ut)(HM)))
#define mbiMA_VZU2U(ut,FM,vz,u)	mbiMM(ut, (FM), mbiA_VZU2U(ut, (vz), (u)))
#define mbiMA_U2VZ(ut,FM,SM,v)	(mbiMM(ut, (FM), (v)) > (ut)(SM)) /* SM==HM */
#define mbiMA_U2M(ut,FM,HM,v)	(mbiMA_U2VZ(ut, (FM), (HM), (v)) ? \
				(ut)((ut)((ut)~(v) & (ut)(HM)) + (ut)1) : \
				(ut)((ut)(v) & (ut)(HM)))

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
#define mbiCAUmul(ut,vl,vr)	do {					\
	if (__predict_false((ut)((ut)((vl) * (vr)) / (vr)) < (vl)))	\
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
 * calculations on manual two’s k̲omplement signed numbers in unsigned vars
 *
 * addition, subtraction and multiplication, bitwise and boolean
 * operations, as well as equality comparisons can be done on the
 * unsigned value; other operations follow:
 */

/* compare < <= > >= */
#define mbiK_signbit(ut,HM)	((ut)((ut)(HM) + (ut)1))
#define mbiK_signflip(ut,HM,v)	((ut)((v) ^ mbiK_signbit(ut, (HM))))
#define mbiKcmp(ut,HM,vl,op,vr)	\
	(mbiK_signflip(ut, (HM), (vl)) op mbiK_signflip(ut, (HM), (vr)))
#define mbiMKcmp(ut,FM,HM,vl,op,vr) \
	mbiKcmp(ut, (HM), mbiMM(ut, (FM), (vl)), op, mbiMM(ut, (FM), (vr)))

/* rotate and shift */
#define mbiK_s1(ut,vl,vr)	(ut)(vl), mbiK_s2(ut, (vr))
#define mbiK_s2(ut,vr)		(vr) & (mbiTYPE_UBITS(ut) - 1U), \
	mbiTYPE_UBITS(ut) - ((vr) & (mbiTYPE_UBITS(ut) - 1U))
#define mbiK_s9(ut,vl,cl,rv)	((cl) ? (ut)(rv) : (ut)(vl))
#define mbiK_rl(ut,vl,cl,CL)	mbiK_s9(ut, (vl), (cl), \
	(ut)((vl) << (cl)) | (ut)((vl) >> (CL)))
#define mbiK_rr(ut,vl,cl,CL)	mbiK_s9(ut, (vl), (cl), \
	(ut)((vl) >> (cl)) | (ut)((vl) << (CL)))
#define mbiK_sl(ut,vl,cl,CL)	mbiK_s9(ut, (vl), (cl), \
	(vl) << (cl))
#define mbiK_sr(ut,vl,cl,CL,vz)	mbiK_s9(ut, (vl), (cl), \
	(vz) ? (ut)~(ut)((ut)~(vl) >> (cl)) : (ut)((vl) >> (cl)))
#define mbiK_rol(ut,args)	mbiK_rl(ut, args)
#define mbiK_ror(ut,args)	mbiK_rr(ut, args)
#define mbiK_shl(ut,args)	mbiK_sl(ut, args)
#define mbiK_sar(ut,args,vz)	mbiK_sr(ut, args, vz)
#define mbiKrol(ut,vl,vr)	mbiK_rol(ut, mbiK_s1(ut, (vl), (vr)))
#define mbiKror(ut,vl,vr)	mbiK_ror(ut, mbiK_s1(ut, (vl), (vr)))
#define mbiKshl(ut,vl,vr)	mbiK_shl(ut, mbiK_s1(ut, (vl), (vr)))
/* let vz be sgn(vl) */
#define mbiKsar(ut,vz,vl,vr)	mbiK_sar(ut, mbiK_s1(ut, (vl), (vr)), (vz))
#define mbiKshr(ut,vl,vr)	mbiK_sar(ut, mbiK_s1(ut, (vl), (vr)), 0)
#define mbiMK_s1(ut,vl,vr)	mbiMM(ut, (FM), (vl)), mbiK_s2(ut, (vr))
#define mbiMKrol(ut,FM,vl,vr)	mbiMM(ut, (FM), \
	mbiK_rol(ut, mbiMK_s1(ut, (FM), (vl), (vr))))
#define mbiMKror(ut,FM,vl,vr)	mbiMM(ut, (FM), \
	mbiK_ror(ut, mbiMK_s1(ut, (FM), (vl), (vr))))
#define mbiMKshl(ut,FM,vl,vr)	mbiMM(ut, (FM), \
	mbiK_shl(ut, mbiMK_s1(ut, (FM), (vl), (vr))))
#define mbiMKsar(ut,FM,vz,l,r)	mbiMM(ut, (FM), \
	mbiK_sar(ut, mbiMK_s1(ut, (FM), (vl), (vr)), (vz)))
#define mbiMKshr(ut,FM,vl,vr)	mbiMM(ut, (FM), \
	mbiK_sar(ut, mbiMK_s1(ut, (FM), (vl), (vr)), 0)

/* division and remainder */
#define mbiKdiv(ut,SM,vl,vr)		mbiA_VZU2U(ut, \
	mbiA_U2VZ(ut, (SM), (vl)) ^ mbiA_U2VZ(ut, (SM), (vr)), \
	mbiA_U2M(ut, (SM), (vl)) / mbiA_U2M(ut, (SM), (vr)))
#define mbiKrem(ut,SM,vl,vr)		((ut)((vl) - \
	(ut)(mbiKdiv(ut, (SM), (vl), (vr)) * (vr))))
#define mbiMK_div(ut,FM,HM,vl,vr)	mbiA_VZU2U(ut, \
	mbiMA_U2VZ(ut, (FM), (HM), (vl)) ^ mbiMA_U2VZ(ut, (FM), (HM), (vr)), \
	mbiMA_U2M(ut, (FM), (HM), (vl)) / mbiMA_U2M(ut, (FM), (HM), (vr)))
#define mbiMKdiv(ut,FM,HM,vl,vr)	mbiMM(ut, (FM), \
	mbiMK_div(ut, (FM), (HM), (vl), (vr)))
#define mbiMKrem(ut,FM,HM,vl,vr)	mbiMM(ut, (FM), (vl) - \
	(ut)(mbiMK_div(ut, (FM), (HM), (vl), (vr)) * (vr)))

#endif /* !SYSKERN_MBSDINT_H */
