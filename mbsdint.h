/*-
 * MirBSD sanitisation attempt for C integer madness
 *
 * © mirabilos Ⓕ CC0 or The MirOS Licence (MirBSD)
 */

#ifndef SYSKERN_MBSDINT_H
#define SYSKERN_MBSDINT_H "$MirOS: src/bin/mksh/mbsdint.h,v 1.25 2023/03/19 22:35:03 tg Exp $"

/*
 * cpp defines to set:
 *
 * -DHAVE_INTCONSTEXPR_RSIZE_MAX=1 if RSIZE_MAX is an integer constexpr
 *
 * -DMBSDINT_H_SMALL_SYSTEM=1 (or 2) to shorten some macros, for old systems
 * or -DMBSDINT_H_SMALL_SYSTEM=3 in very tiny address space (< 32 bits)
 *
 * -DMBSDINT_H_WANT_SIZET_IN_LONG (breaks LLP64) ensure pointers fit storage
 * -DMBSDINT_H_WANT_INT32=1 to ensure unsigned int has at least 32 bit width
 * -DMBSDINT_H_WANT_LRG64=1 to ensure mbiLARGE_U has at least 64 bit width
 * (these trigger extra compile-time assertion checks)
 *
 * If <sys/types.h>, <inttypes.h> and/or <stdint.h> exist, INCLUDE THEM
 * BEFORE this header; also prerequisites of <limits.h> and <stddef.h>,
 * which this header includes always.
 */

#if !defined(_KERNEL) && !defined(_STANDALONE)
#include <limits.h>
#include <stddef.h>
#else
#include <sys/cdefs.h>
#include <sys/limits.h>
#include <sys/stddef.h>
#endif

#if !defined(MBSDINT_H_SMALL_SYSTEM) || ((MBSDINT_H_SMALL_SYSTEM) < 1)
#undef MBSDINT_H_SMALL_SYSTEM	/* in case someone defined it as 0 */
#define mbiMASK_bitmax		279
#elif (MBSDINT_H_SMALL_SYSTEM) >= 2
#define mbiMASK_bitmax		62
#else
#define mbiMASK_bitmax		93
#endif

#if defined(MBSDINT_H_WANT_SIZET_IN_LONG) && ((MBSDINT_H_WANT_SIZET_IN_LONG) < 1)
#undef MBSDINT_H_WANT_SIZET_IN_LONG
#endif
#if defined(MBSDINT_H_WANT_INT32) && ((MBSDINT_H_WANT_INT32) < 1)
#undef MBSDINT_H_WANT_INT32
#endif
#if defined(MBSDINT_H_WANT_LRG64) && ((MBSDINT_H_WANT_LRG64) < 1)
#undef MBSDINT_H_WANT_LRG64
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
#define mbiCTA_TYPE_MBIT(nm,ty)	char ctatm_ ## nm [\
	(sizeof(ty) <= (mbiMASK_bitmax / (CHAR_BIT))) ? 1 : -1]

/* kinds of types */
	/* runtime only, but see mbiCTA_TYPE_NOTF */
#define mbiTYPE_ISF(type)	((double)0.5 == (double)(type)0.5)
	/* compile-time and runtime */
#define mbiTYPE_ISU(type)	((type)-1 > (type)0)
/* limits of types */
#define mbiTYPE_UMAX(type)	((type)~(type)0U)
#define mbiTYPE_UBITS(type)	mbiMASK_BITS(mbiTYPE_UMAX(type))
/* calculation by Hallvard B Furuseth (via comp.lang.c), ≤ 2039 bits */
#define mbiMASK__BITS(maxv)	((maxv) / ((maxv) % 255 + 1) / \
				255 % 255 * 8 + 7 - 86 / ((maxv) % 255 + 12))
#define mbiMASK_BITS(maxv)	((unsigned int)mbiMASK__BITS(maxv))

/* ensure v is a positive (2ⁿ-1) number (n>0), up to 279 (or less) bits */
#define mbiMASK_CHK(v)		((v) > 0 ? mbi_maskchk31_1((v)) : 0)
#define mbi_maskchks(v,m,o,n)	(v <= m ? o : ((v & m) == m) && n)
#define mbi_maskchk31s(v,n)	mbi_maskchks(v, 0x7FFFFFFFUL, \
				    mbi_maskchk16(v), n)
#if !defined(MBSDINT_H_SMALL_SYSTEM)
#define mbi_maskchk31_1(v)	mbi_maskchk31s(v, mbi_maskchk31_2(v >> 31))
#define mbi_maskchk31_2(v)	mbi_maskchk31s(v, mbi_maskchk31_3(v >> 31))
#define mbi_maskchk31_3(v)	mbi_maskchk31s(v, mbi_maskchk31_4(v >> 31))
#define mbi_maskchk31_4(v)	mbi_maskchk31s(v, mbi_maskchk31_5(v >> 31))
#define mbi_maskchk31_5(v)	mbi_maskchk31s(v, mbi_maskchk31_6(v >> 31))
#define mbi_maskchk31_6(v)	mbi_maskchk31s(v, mbi_maskchk31_7(v >> 31))
#define mbi_maskchk31_7(v)	mbi_maskchk31s(v, mbi_maskchk31_8(v >> 31))
#define mbi_maskchk31_8(v)	mbi_maskchk31s(v, mbi_maskchk31_9(v >> 31))
#elif (MBSDINT_H_SMALL_SYSTEM) >= 2
#define mbi_maskchk31_1(v)	mbi_maskchk31s(v, mbi_maskchk31_9(v >> 31))
#else /* (MBSDINT_H_SMALL_SYSTEM) == 1 */
#define mbi_maskchk31_1(v)	mbi_maskchk31s(v, mbi_maskchk31_8(v >> 31))
#define mbi_maskchk31_8(v)	mbi_maskchk31s(v, mbi_maskchk31_9(v >> 31))
#endif
#define mbi_maskchk31_9(v)	(v <= 0x7FFFFFFFUL && mbi_maskchk16(v))
#define mbi_maskchk16(v)	mbi_maskchks(v, 0xFFFFU, \
				    mbi_maskchk8(v), mbi_maskchk8(v >> 16))
#define mbi_maskchk8(v)		mbi_maskchks(v, 0xFFU, \
				    mbi_maskchk4(v), mbi_maskchk4(v >> 8))
#define mbi_maskchk4(v)		mbi_maskchks(v, 0xFU, \
				    mbi_maskchkF(v), mbi_maskchkF(v >> 4))
#define mbi_maskchkF(v)		(v == 0xF || v == 7 || v == 3 || v == 1 || !v)

/* limit maximum object size so we can express pointer differences w/o UB */
#if defined(HAVE_INTCONSTEXPR_RSIZE_MAX) && (HAVE_INTCONSTEXPR_RSIZE_MAX != 0)
/* trust but verify operating environment */
#define mbiRSZCHK	RSIZE_MAX
#define mbiSIZEMAX	((size_t)RSIZE_MAX)
#elif !defined(SIZE_MAX)
#ifdef PTRDIFF_MAX
#define mbiRSZCHK	PTRDIFF_MAX
#define mbiSIZEMAX	((size_t)PTRDIFF_MAX)
#elif defined(MBSDINT_H_SMALL_SYSTEM) && ((MBSDINT_H_SMALL_SYSTEM) >= 3)
#define mbiSIZEMAX	mbiTYPE_UMAX(size_t)
#else
#define mbiSIZEMAX	((size_t)(mbiTYPE_UMAX(size_t) >> 1))
#endif /* !PTRDIFF_MAX !MBSDINT_H_SMALL_SYSTEM>=3 */
#elif !defined(PTRDIFF_MAX) || ((SIZE_MAX) == (PTRDIFF_MAX))
#if defined(MBSDINT_H_SMALL_SYSTEM) && ((MBSDINT_H_SMALL_SYSTEM) >= 3)
#define mbiSIZEMAX	((size_t)SIZE_MAX)
#else
#define mbiSIZEMAX	((size_t)(((size_t)SIZE_MAX) >> 1))
#endif
#elif (SIZE_MAX) < (PTRDIFF_MAX)
#define mbiSIZEMAX	((size_t)SIZE_MAX)
#else
#define mbiRSZCHK	PTRDIFF_MAX
#define mbiSIZEMAX	((size_t)PTRDIFF_MAX)
#endif

/* largest integer */
#if defined(INTMAX_MIN)
#define mbiHUGE_S		intmax_t
#define mbiHUGE_MIN		INTMAX_MIN
#define mbiHUGE_MAX		INTMAX_MAX
#define mbiHUGE_U		uintmax_t
#define mbiHUGE_UMAX		UINTMAX_MAX
#define mbiHUGE_P(c)		PRI ## c ## MAX
#elif defined(LLONG_MIN)
#define mbiHUGE_S		long long
#define mbiHUGE_MIN		LLONG_MIN
#define mbiHUGE_MAX		LLONG_MAX
#define mbiHUGE_U		unsigned long long
#define mbiHUGE_UMAX		ULLONG_MAX
#define mbiHUGE_P(c)		"ll" #c
#elif defined(_UI64_MAX) && (mbiMASK__BITS(_UI64_MAX) > mbiMASK__BITS(ULONG_MAX))
#define mbiHUGE_S		signed __int64
#define mbiHUGE_MIN		_I64_MIN
#define mbiHUGE_MAX		_I64_MAX
#define mbiHUGE_U		unsigned __int64
#define mbiHUGE_UMAX		_UI64_MAX
#define mbiHUGE_P(c)		"I64" #c
#else /* C89? */
#define mbiHUGE_S		long
#define mbiHUGE_MIN		LONG_MIN
#define mbiHUGE_MAX		LONG_MAX
#define mbiHUGE_U		unsigned long
#define mbiHUGE_UMAX		ULONG_MAX
#define mbiHUGE_P(c)		"l" #c
#endif
/* larger than int, 64 bit if possible, storage fits size_t/pointers */
#if (mbiMASK__BITS(ULONG_MAX) >= 64)
#define mbiLARGE_S		long
#define mbiLARGE_MIN		LONG_MIN
#define mbiLARGE_MAX		LONG_MAX
#define mbiLARGE_U		unsigned long
#define mbiLARGE_UMAX		ULONG_MAX
#define mbiLARGE_P(c)		"l" #c
#elif defined(ULLONG_MAX) && (mbiMASK__BITS(ULLONG_MAX) >= 64)
#define mbiLARGE_S		long long
#define mbiLARGE_MIN		LLONG_MIN
#define mbiLARGE_MAX		LLONG_MAX
#define mbiLARGE_U		unsigned long long
#define mbiLARGE_UMAX		ULLONG_MAX
#define mbiLARGE_P(c)		"ll" #c
#elif defined(_UI64_MAX) && (mbiMASK__BITS(_UI64_MAX) >= 64)
#define mbiLARGE_S		signed __int64
#define mbiLARGE_MIN		_I64_MIN
#define mbiLARGE_MAX		_I64_MAX
#define mbiLARGE_U		unsigned __int64
#define mbiLARGE_UMAX		_UI64_MAX
#define mbiLARGE_P(c)		"I64" #c
#else
#define mbiLARGE_S		mbiHUGE_S
#define mbiLARGE_MIN		mbiHUGE_MIN
#define mbiLARGE_MAX		mbiHUGE_MAX
#define mbiLARGE_U		mbiHUGE_U
#define mbiLARGE_UMAX		mbiHUGE_UMAX
#define mbiLARGE_P		mbiHUGE_P
#endif

#ifndef MBSDINT_H_SKIP_CTAS
/* compile-time assertions for mbsdint.h */
mbiCTAS(mbsdint_h) {
 /* compiler (in)sanity, from autoconf */
#define mbiCTf(x) 'x'
 mbiCTA(xlc6, mbiCTf(a) == 'x');
#undef mbiCTf
 mbiCTA(osf4, '\x00' == 0);
 /* C types */
 mbiCTA(basic_char_smask, mbiMASK_CHK(SCHAR_MAX));
 mbiCTA(basic_char_umask, mbiMASK_CHK(UCHAR_MAX));
 mbiCTA(basic_char,
	sizeof(char) == 1 && (CHAR_BIT) >= 7 && (CHAR_BIT) < 2040 &&
	(((CHAR_MAX) == (SCHAR_MAX) && (CHAR_MIN) == (SCHAR_MIN)) ||
	 ((CHAR_MAX) == (UCHAR_MAX) && (CHAR_MIN) == 0)) &&
	mbiTYPE_UMAX(unsigned char) == (UCHAR_MAX) &&
	sizeof(signed char) == 1 &&
	sizeof(unsigned char) == 1 &&
	mbiTYPE_UBITS(unsigned char) >= 8 && (SCHAR_MIN) < 0 &&
	((SCHAR_MIN) == -(SCHAR_MAX) || (SCHAR_MIN)+1 == -(SCHAR_MAX)) &&
	mbiTYPE_UBITS(unsigned char) == (unsigned int)(CHAR_BIT));
 mbiCTA_TYPE_MBIT(short, short);
 mbiCTA_TYPE_MBIT(ushort, unsigned short);
 mbiCTA_TYPE_MBIT(int, int);
 mbiCTA_TYPE_MBIT(uint, unsigned int);
 mbiCTA_TYPE_MBIT(long, long);
 mbiCTA_TYPE_MBIT(ulong, unsigned long);
#ifdef LLONG_MIN
 mbiCTA_TYPE_MBIT(llong, long long);
 mbiCTA_TYPE_MBIT(ullong, unsigned long long);
#endif
#ifdef INTMAX_MIN
 mbiCTA_TYPE_MBIT(imax, intmax_t);
 mbiCTA_TYPE_MBIT(uimax, uintmax_t);
#endif
#ifdef _UI64_MAX
 mbiCTA_TYPE_MBIT(ms_i64, signed __int64);
 mbiCTA_TYPE_MBIT(ms_ui64, unsigned __int64);
#endif
 mbiCTA(basic_short_smask, mbiMASK_CHK(SHRT_MAX));
 mbiCTA(basic_short_umask, mbiMASK_CHK(USHRT_MAX));
 mbiCTA(basic_short,
	mbiTYPE_UMAX(unsigned short) == (USHRT_MAX) &&
	mbiTYPE_UBITS(unsigned short) >= 16 && (SHRT_MIN) < 0 &&
	((SHRT_MIN) == -(SHRT_MAX) || (SHRT_MIN)+1 == -(SHRT_MAX)) &&
	sizeof(short) >= sizeof(signed char) &&
	sizeof(unsigned short) >= sizeof(unsigned char) &&
	mbiMASK_BITS(SHRT_MAX) >= mbiMASK_BITS(SCHAR_MAX) &&
	mbiMASK_BITS(SHRT_MAX) < mbiMASK_BITS(mbiHUGE_MAX) &&
	mbiMASK_BITS(USHRT_MAX) >= mbiMASK_BITS(UCHAR_MAX) &&
	mbiMASK_BITS(USHRT_MAX) < mbiMASK_BITS(mbiHUGE_UMAX) &&
	sizeof(short) == sizeof(unsigned short));
 mbiCTA(basic_int_smask, mbiMASK_CHK(INT_MAX));
 mbiCTA(basic_int_umask, mbiMASK_CHK(UINT_MAX));
 mbiCTA(basic_int,
	mbiTYPE_UMAX(unsigned int) == (UINT_MAX) &&
	mbiTYPE_UBITS(unsigned int) >= 16 && (INT_MIN) < 0 &&
	((INT_MIN) == -(INT_MAX) || (INT_MIN)+1 == -(INT_MAX)) &&
	sizeof(int) >= sizeof(short) &&
	sizeof(unsigned int) >= sizeof(unsigned short) &&
	mbiMASK_BITS(INT_MAX) >= mbiMASK_BITS(SHRT_MAX) &&
	mbiMASK_BITS(INT_MAX) <= mbiMASK_BITS(mbiHUGE_MAX) &&
	mbiMASK_BITS(UINT_MAX) >= mbiMASK_BITS(USHRT_MAX) &&
	mbiMASK_BITS(UINT_MAX) <= mbiMASK_BITS(mbiHUGE_UMAX) &&
	sizeof(int) == sizeof(unsigned int));
 mbiCTA(basic_long_smask, mbiMASK_CHK(LONG_MAX));
 mbiCTA(basic_long_umask, mbiMASK_CHK(ULONG_MAX));
 mbiCTA(basic_long,
	mbiTYPE_UMAX(unsigned long) == (ULONG_MAX) &&
	mbiTYPE_UBITS(unsigned long) >= 32 && (LONG_MIN) < 0 &&
	((LONG_MIN) == -(LONG_MAX) || (LONG_MIN)+1 == -(LONG_MAX)) &&
	sizeof(long) >= sizeof(int) &&
	sizeof(unsigned long) >= sizeof(unsigned int) &&
	mbiMASK_BITS(LONG_MAX) >= mbiMASK_BITS(INT_MAX) &&
	mbiMASK_BITS(LONG_MAX) <= mbiMASK_BITS(mbiHUGE_MAX) &&
	mbiMASK_BITS(ULONG_MAX) >= mbiMASK_BITS(UINT_MAX) &&
	mbiMASK_BITS(ULONG_MAX) <= mbiMASK_BITS(mbiHUGE_UMAX) &&
	sizeof(long) == sizeof(unsigned long));
#ifdef LLONG_MIN
 mbiCTA(basic_quad_smask, mbiMASK_CHK(LLONG_MAX));
 mbiCTA(basic_quad_umask, mbiMASK_CHK(ULLONG_MAX));
 mbiCTA(basic_quad,
	mbiTYPE_UMAX(unsigned long long) == (ULLONG_MAX) &&
	mbiTYPE_UBITS(unsigned long long) >= 32 && (LLONG_MIN) < 0 &&
	((LLONG_MIN) == -(LLONG_MAX) || (LLONG_MIN)+1 == -(LLONG_MAX)) &&
	sizeof(long long) >= sizeof(long) &&
	sizeof(unsigned long long) >= sizeof(unsigned long) &&
	mbiMASK_BITS(LLONG_MAX) >= mbiMASK_BITS(LONG_MAX) &&
	mbiMASK_BITS(LLONG_MAX) <= mbiMASK_BITS(mbiHUGE_MAX) &&
	mbiMASK_BITS(ULLONG_MAX) >= mbiMASK_BITS(ULONG_MAX) &&
	mbiMASK_BITS(ULLONG_MAX) <= mbiMASK_BITS(mbiHUGE_UMAX) &&
	sizeof(long long) == sizeof(unsigned long long));
#endif
#ifdef INTMAX_MIN
 mbiCTA(basic_imax_smask, mbiMASK_CHK(INTMAX_MAX));
 mbiCTA(basic_imax_umask, mbiMASK_CHK(UINTMAX_MAX));
 mbiCTA(basic_imax,
	mbiTYPE_UMAX(uintmax_t) == (UINTMAX_MAX) &&
	mbiTYPE_UBITS(uintmax_t) >= 32 && (INTMAX_MIN) < 0 &&
	((INTMAX_MIN) == -(INTMAX_MAX) || (INTMAX_MIN)+1 == -(INTMAX_MAX)) &&
	sizeof(intmax_t) >= sizeof(long) &&
	sizeof(uintmax_t) >= sizeof(unsigned long) &&
	mbiMASK_BITS(INTMAX_MAX) >= mbiMASK_BITS(LONG_MAX) &&
	mbiMASK_BITS(INTMAX_MAX) <= mbiMASK_BITS(mbiHUGE_MAX) &&
	mbiMASK_BITS(UINTMAX_MAX) >= mbiMASK_BITS(ULONG_MAX) &&
	mbiMASK_BITS(UINTMAX_MAX) <= mbiMASK_BITS(mbiHUGE_UMAX) &&
	sizeof(intmax_t) == sizeof(uintmax_t));
#ifdef LLONG_MIN
 mbiCTA(basic_imax_llong,
	mbiMASK_BITS(INTMAX_MAX) >= mbiMASK_BITS(LLONG_MAX) &&
	mbiMASK_BITS(UINTMAX_MAX) >= mbiMASK_BITS(ULLONG_MAX) &&
	sizeof(intmax_t) >= sizeof(long long) &&
	sizeof(uintmax_t) >= sizeof(unsigned long long));
#endif /* INTMAX_MIN && LLONG_MIN */
#endif /* INTMAX_MIN */
 mbiCTA(basic_large_smask, mbiMASK_CHK(mbiLARGE_MAX));
 mbiCTA(basic_large_umask, mbiMASK_CHK(mbiLARGE_UMAX));
 mbiCTA(basic_large,
	mbiTYPE_ISU(mbiLARGE_U) && !mbiTYPE_ISU(mbiLARGE_S) &&
	mbiTYPE_UMAX(mbiLARGE_U) == (mbiLARGE_UMAX) && (mbiLARGE_MIN) < 0 &&
	((mbiLARGE_MIN) == -(mbiLARGE_MAX) || (mbiLARGE_MIN)+1 == -(mbiLARGE_MAX)) &&
	sizeof(mbiLARGE_S) >= sizeof(long) &&
	sizeof(mbiLARGE_U) >= sizeof(unsigned long) &&
	mbiMASK_BITS(mbiLARGE_MAX) >= mbiMASK_BITS(LONG_MAX) &&
	mbiMASK_BITS(mbiLARGE_MAX) <= mbiMASK_BITS(mbiHUGE_MAX) &&
	mbiMASK_BITS(mbiLARGE_UMAX) >= mbiMASK_BITS(ULONG_MAX) &&
	mbiMASK_BITS(mbiLARGE_UMAX) <= mbiMASK_BITS(mbiHUGE_UMAX) &&
	sizeof(mbiLARGE_S) == sizeof(mbiLARGE_U));
 mbiCTA(basic_huge_smask, mbiMASK_CHK(mbiHUGE_MAX));
 mbiCTA(basic_huge_umask, mbiMASK_CHK(mbiHUGE_UMAX));
 mbiCTA(basic_huge,
	mbiTYPE_ISU(mbiHUGE_U) && !mbiTYPE_ISU(mbiHUGE_S) &&
	mbiTYPE_UMAX(mbiHUGE_U) == (mbiHUGE_UMAX) && (mbiHUGE_MIN) < 0 &&
	((mbiHUGE_MIN) == -(mbiHUGE_MAX) || (mbiHUGE_MIN)+1 == -(mbiHUGE_MAX)) &&
	sizeof(mbiHUGE_S) >= sizeof(long) &&
	sizeof(mbiHUGE_U) >= sizeof(unsigned long) &&
	mbiMASK_BITS(mbiHUGE_MAX) >= mbiMASK_BITS(LONG_MAX) &&
	mbiMASK_BITS(mbiHUGE_UMAX) >= mbiMASK_BITS(ULONG_MAX) &&
	sizeof(mbiHUGE_S) == sizeof(mbiHUGE_U));
 /* size_t is C but ssize_t is POSIX */
 mbiCTA_TYPE_NOTF(size_t);
 mbiCTA(basic_sizet,
	sizeof(size_t) >= sizeof(unsigned int) &&
	sizeof(size_t) <= sizeof(mbiHUGE_U) &&
	mbiTYPE_ISU(size_t) &&
	mbiTYPE_UBITS(size_t) >= mbiMASK_BITS(UINT_MAX) &&
	mbiTYPE_UBITS(size_t) <= mbiMASK_BITS(mbiHUGE_UMAX));
#ifdef SIZE_MAX
 mbiCTA(basic_sizet_mask, mbiMASK_CHK(SIZE_MAX));
 mbiCTA(basic_sizet_max,
	mbiMASK_BITS(SIZE_MAX) <= mbiTYPE_UBITS(size_t) &&
	mbiMASK_BITS(SIZE_MAX) >= mbiMASK_BITS(USHRT_MAX) &&
	((mbiHUGE_U)(SIZE_MAX) == (mbiHUGE_U)(size_t)(SIZE_MAX)));
#endif
 /* note mbiSIZEMAX is not necessarily a mask (0b0*1+) */
#ifdef mbiRSZCHK
 mbiCTA(smax_check, ((mbiHUGE_U)(mbiRSZCHK) == (mbiHUGE_U)(size_t)(mbiRSZCHK)));
#undef mbiRSZCHK
#endif
 mbiCTA_TYPE_NOTF(ptrdiff_t);
 mbiCTA(basic_ptrdifft,
	sizeof(ptrdiff_t) >= sizeof(int) &&
	sizeof(ptrdiff_t) <= sizeof(mbiHUGE_S) &&
	!mbiTYPE_ISU(ptrdiff_t));
#ifdef PTRDIFF_MAX
 mbiCTA(basic_ptrdifft_mask, mbiMASK_CHK(PTRDIFF_MAX));
 mbiCTA(basic_ptrdifft_max,
	mbiMASK_BITS(PTRDIFF_MAX) >= mbiMASK_BITS(INT_MAX) &&
	mbiMASK_BITS(PTRDIFF_MAX) <= mbiMASK_BITS(mbiHUGE_MAX) &&
	((mbiHUGE_S)(PTRDIFF_MAX) == (mbiHUGE_S)(ptrdiff_t)(PTRDIFF_MAX)));
#endif
 /* UGH! off_t is POSIX, with no _MIN/_MAX constants… WTF‽ */
#ifdef SSIZE_MAX
 mbiCTA_TYPE_NOTF(ssize_t);
 mbiCTA(basic_ssizet_mask, mbiMASK_CHK(SSIZE_MAX));
 mbiCTA(basic_ssizet,
	sizeof(ssize_t) == sizeof(size_t) &&
	!mbiTYPE_ISU(ssize_t) &&
	mbiMASK_BITS(SSIZE_MAX) >= mbiMASK_BITS(INT_MAX) &&
	mbiMASK_BITS(SSIZE_MAX) <= mbiMASK_BITS(mbiHUGE_MAX) &&
	((mbiHUGE_S)(SSIZE_MAX) == (mbiHUGE_S)(ssize_t)(SSIZE_MAX)) &&
	mbiMASK_BITS(SSIZE_MAX) < mbiTYPE_UBITS(size_t));
#ifdef SIZE_MAX
 mbiCTA(basic_ssizet_sizet,
	mbiMASK_BITS(SSIZE_MAX) <= mbiMASK_BITS(SIZE_MAX));
#endif /* SSIZE_MAX && SIZE_MAX */
#endif /* SSIZE_MAX */
#ifdef UINTPTR_MAX
 mbiCTA(basic_uintptr_mask, mbiMASK_CHK(UINTPTR_MAX));
 mbiCTA(basic_uintptr,
	sizeof(uintptr_t) >= sizeof(ptrdiff_t) &&
	sizeof(uintptr_t) >= sizeof(size_t) &&
	mbiTYPE_ISU(uintptr_t) &&
	mbiMASK_BITS(UINTPTR_MAX) == mbiTYPE_UBITS(uintptr_t) &&
	((mbiHUGE_U)(UINTPTR_MAX) == (mbiHUGE_U)(uintptr_t)(UINTPTR_MAX)) &&
	mbiTYPE_UBITS(uintptr_t) >= mbiMASK_BITS(UINT_MAX) &&
	mbiTYPE_UBITS(uintptr_t) <= mbiMASK_BITS(mbiHUGE_UMAX));
#ifdef PTRDIFF_MAX
 mbiCTA(basic_uintptr_pdt,
	mbiMASK_BITS(UINTPTR_MAX) >= mbiMASK_BITS(PTRDIFF_MAX));
#endif /* UINTPTR_MAX && PTRDIFF_MAX */
#ifdef SIZE_MAX
 mbiCTA(basic_uintptr_sizet,
	mbiMASK_BITS(UINTPTR_MAX) >= mbiMASK_BITS(SIZE_MAX));
#endif /* UINTPTR_MAX && SIZE_MAX */
#endif /* UINTPTR_MAX */
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
 mbiCTA(vbits_large, mbiMASK_BITS(mbiLARGE_UMAX) == mbiMASK_BITS(mbiLARGE_MAX) + 1U);
 mbiCTA(vbits_huge, mbiMASK_BITS(mbiHUGE_UMAX) == mbiMASK_BITS(mbiHUGE_MAX) + 1U);
#ifdef SSIZE_MAX
 mbiCTA(vbits_size, mbiTYPE_UBITS(size_t) == mbiMASK_BITS(SSIZE_MAX) + 1U);
#endif
#if defined(UINTPTR_MAX) && defined(PTRDIFF_MAX)
 mbiCTA(vbits_iptr, mbiMASK_BITS(UINTPTR_MAX) == mbiMASK_BITS(PTRDIFF_MAX) + 1U);
#endif
 /* require pointers and size_t to take up the same amount of space */
 mbiCTA(sizet_voidptr, sizeof(size_t) == sizeof(void *));
 mbiCTA(sizet_sintptr, sizeof(size_t) == sizeof(int *));
 mbiCTA(sizet_funcptr, sizeof(size_t) == sizeof(void (*)(void)));
 /* do size_t and ulong fit each other? */
 mbiCTA(sizet_minlong, sizeof(size_t) >= sizeof(long));
#ifdef MBSDINT_H_WANT_SIZET_IN_LONG /* breaks LLP64 (e.g. Windows/amd64) */
 mbiCTA(sizet_inulong, sizeof(size_t) <= sizeof(long));
#endif
 mbiCTA(sizet_inlarge, sizeof(size_t) <= sizeof(mbiLARGE_U));
 /* assume ptrdiff_t and (s)size_t will fit each other */
 mbiCTA(sizet_ptrdiff, sizeof(size_t) == sizeof(ptrdiff_t));
#ifdef SSIZE_MAX
 mbiCTA(sizet_ssize, sizeof(size_t) == sizeof(ssize_t));
#endif
 /* also uintptr_t, to rule out certain weird machines */
#ifdef UINTPTR_MAX
 mbiCTA(sizet_uintptr, sizeof(size_t) == sizeof(uintptr_t));
#endif
 /* user-requested extra checks */
#ifdef MBSDINT_H_WANT_INT32
 mbiCTA(user_int32, mbiTYPE_UBITS(unsigned int) >= 32);
#endif
#ifdef MBSDINT_H_WANT_LRG64
 mbiCTA(user_lrg64, mbiTYPE_UBITS(mbiLARGE_U) >= 64);
#endif
};
#endif /* !MBSDINT_H_SKIP_CTAS */

/*
 * various arithmetics, optionally masking
 *
 * key:	O = operation, A = arithmetic, M* = masking
 *	U = unsigned, S = signed, m = magnitude
 *	VZ = sign (Vorzeichen) 0=positive, 1=negative
 *	v = value, ut = unsigned type, st = signed type
 *	w = boolean value, y = yes, n = no
 *	CA = checked arithmetics, define mbiCfail
 *	K = k̲alculations for unsigned mantwo’s k̲omplement vars
 *
 * masks: SM = signed maximum (e.g. LONG_MAX), equivalent to…
 *	  HM = half mask (usually FM >> 1 or 0x7FFFFFFFUL)
 *	  FM = full mask (e.g. ULONG_MAX or 0xFFFFFFFFUL)
 *	  tM = type-corresponding mask
 *
 * Where unsigned values may be either properly unsigned or
 * manually represented in two’s complement (M=N-1 above),
 * signed values are the host’s (and conversion will UB for
 * -type_MAX-1 on one’s complement or sign-and-magnitude,
 * so don’t use that without checking), unsigned magnitude
 * ranges from 0 to type_MAX+1, and we try pretty hard to
 * avoid UB and (with one exception, below, casting into
 * smaller-width signed integers, checked to be safe, but
 * an implementation-defined signal may be raised, which is
 * then the caller’s responsibility) IB.
 *
 * Every operation needs casting applied, as the type may
 * have lower integer rank than (signed) int. Additional
 * masking may be used to guarantee bit widths in the result
 * even when the executing type is wider. In conversion, if
 * unsigned and magnitude both exist, magnitude is halfmasked
 * whereas unsigned is fullmasked with vz acting as neg op.
 */

/* basic building blocks: arithmetics cast helpers */

#define mbiUI(v)		(0U + (v))
#define mbiUP(ut,v)		mbiUI((ut)(v))
#define mbiSP(st,v)		(0 + ((st)(v)))

/* basic building blocks: masking */

#define mbiMM(ut,tM,v)		mbiOU(ut, (v), &, (tM))

/* basic building blocks: operations */

/* 1. unary */
#define mbiOS1(st,op,v)		((st)(op mbiSP(st, (v))))
#define mbiOU1(ut,op,v)		((ut)(op mbiUP(ut, (v))))
#define mbiMO1(ut,tM,op,v)	mbiMM(ut, (tM), mbiOU1(ut, op, (v)))

/* 2. binary or comparison */
#define mbiOS(st,l,op,r)	((st)(mbiSP(st, (l)) op mbiSP(st, (r))))
#define mbiOU(ut,l,op,r)	((ut)(mbiUP(ut, (l)) op mbiUP(ut, (r))))
#define mbiMO(ut,tM,l,op,r)	mbiMM(ut, (tM), mbiOU(ut, (l), op, (r)))

#define mbiOshl(ut,l,r)		((ut)(mbiUP(ut, (l)) << (r)))
#define mbiOshr(ut,l,r)		((ut)(mbiUP(ut, (l)) >> (r)))

#define mbiCOS(st,l,op,r)	(!!(mbiSP(st, (l)) op mbiSP(st, (r))))
#define mbiCOU(ut,l,op,r)	(!!(mbiUP(ut, (l)) op mbiUP(ut, (r))))

/* 3. ternary */
#define mbiOT(t,w,j,n)		((w) ? (t)(j) : (t)(n))
#define mbiMOT(ut,tM,w,j,n)	mbiMM(ut, (tM), mbiOT(ut, (w), (j), (n)))

/* manual two’s complement in unsigned arithmetics */

/* 1. obtain sign of encoded value */
#define mbiA_S2VZ(v)		((v) < 0)
#define mbiA_U2VZ(ut,SM,v)	mbiCOU(ut, (v), >, (SM))
/* masking; recall SM==HM */
#define mbiMA_U2VZ(ut,FM,SM,v)	mbiCOU(ut, mbiMM(ut, (FM), (v)), >, (SM))

/* 2. casting between unsigned(two’s complement) and signed(native) */
#define mbiA_U2S(ut,st,SM,v)						\
		mbiOT(st,						\
		 mbiA_U2VZ(ut, (SM), (v)),				\
		 mbiOS(st,						\
		  mbiOS1(st,						\
		   -,							\
		   mbiOU1(ut, ~, (v))					\
		  ),							\
		  -,							\
		  1							\
		 ),							\
		 (v)							\
		)
#define mbiMA_U2S(ut,st,FM,HM,v)					\
		mbiOT(st,						\
		 mbiMA_U2VZ(ut, (FM), (HM), (v)),			\
		 mbiOS(st,						\
		  mbiOS1(st,						\
		   -,							\
		   mbiMO1(ut, (HM), ~, (v))				\
		  ),							\
		  -,							\
		  1							\
		 ),							\
		 mbiMM(ut, (HM), (v))					\
		)

#define mbiA_S2U(ut,st,v)						\
		mbiOT(ut,						\
		 mbiA_S2VZ(v),						\
		 mbiOU1(ut,						\
		  ~,							\
		  mbiOS1(st,						\
		   -,							\
		   mbiOS(st, (v), +, 1)					\
		  )							\
		 ),							\
		 (v)							\
		)
#define mbiMA_S2U(ut,st,FM,v)	mbiMM(ut, (FM), mbiA_S2U(ut, st, (v)))

/* 3. converting from signed(native) or unsigned(manual) to magnitude */
#define mbiA_S2M(ut,st,v)						\
		mbiOT(ut,						\
		 mbiA_S2VZ(v),						\
		 mbiOU(ut,						\
		  mbiOS1(st,						\
		   -,							\
		   mbiOS(st, (v), +, 1)					\
		  ),							\
		  +,							\
		  1U							\
		 ),							\
		 (v)							\
		)
#define mbiMA_S2M(ut,st,HM,v)						\
		mbiOT(ut,						\
		 mbiA_S2VZ(v),						\
		 mbiOU(ut,						\
		  mbiMM(ut, (HM),					\
		   mbiOS1(st,						\
		    -,							\
		    mbiOS(st, (v), +, 1)				\
		   )							\
		  ),							\
		  +,							\
		  1U							\
		 ),							\
		 mbiMM(ut, (HM), (v))					\
		)

#define mbiA_U2M(ut,SM,v)						\
		mbiOT(ut,						\
		 mbiA_U2VZ(ut, (SM), (v)),				\
		 mbiOU1(ut, -, (v)),					\
		 (v)							\
		)
#define mbiMA_U2M(ut,FM,HM,v)						\
		mbiOT(ut,						\
		 mbiMA_U2VZ(ut, (FM), (HM), (v)),			\
		 mbiOU(ut,						\
		  mbiMO1(ut, (HM), ~, (v)),				\
		  +,							\
		  1U							\
		 ),							\
		 mbiMM(ut, (HM), (v))					\
		)

/* 4a. reverse: signbit plus magnitude to signed(native) */
#define mbiA_VZM2S(ut,st,vz,m)						\
		mbiOT(st,						\
		 (vz) && ((m) > 0U),					\
		 mbiOS(st,						\
		  mbiOS1(st,						\
		   -,							\
		   mbiOU(ut, (m), -, 1U)				\
		  ),							\
		  -,							\
		  1							\
		 ),							\
		 (m)							\
		)
#define mbiMA_VZM2S(ut,st,FM,HM,vz,m)					\
		mbiOT(st,						\
		 (vz) && (mbiMM(ut, (FM), (m)) > 0U),			\
		 mbiOS(st,						\
		  mbiOS1(st,						\
		   -,							\
		   mbiMO(ut, (HM), (m), -, 1U)				\
		  ),							\
		  -,							\
		  1							\
		 ),							\
		 mbiMM(ut, (HM), (m))					\
		)

/* 4b. same: signbit and magnitude(masked) to unsigned(two’s complement) */
#define mbiA_VZM2U(ut,SM,vz,m)						\
		mbiOT(ut,						\
		 (vz) && ((m) > 0U),					\
		 mbiOU1(ut,						\
		  ~,							\
		  mbiMO(ut, (SM), (m), -, 1U)				\
		 ),							\
		 mbiMM(ut, (SM), (m))					\
		)
#define mbiMA_VZM2U(ut,FM,HM,vz,m)					\
		mbiOT(ut,						\
		 (vz) && (mbiMM(ut, (FM), (m)) > 0U),			\
		 mbiMO1(ut, (FM),					\
		  ~,							\
		  mbiMO(ut, (HM), (m), -, 1U)				\
		 ),							\
		 mbiMM(ut, (HM), (m))					\
		)

/* 4c. signbit and unsigned to unsigned(two’s complement), basically NEG */
#define mbiA_VZU2U(ut,vz,m)						\
		mbiOT(ut,						\
		 (vz),							\
		 mbiOU1(ut, -, (m)),					\
		 (m)							\
		)
#define mbiMA_VZU2U(ut,FM,vz,u)	mbiMM(ut, (FM), mbiA_VZU2U(ut, (vz), (u)))

/*
 * UB-safe overflow/underflow-checking integer arithmetics
 *
 * Before using, #define mbiCfail to the action that should be
 * taken on check (e.g. "goto fail" or "return (0)"); make sure
 * to #undef mbiCfail before the end of the function body though!
 */

/* 0. internal helpers */
#define mbi_halftmax(stmax)	\
	((((1 ? 0 : (stmax)) + 1) << (mbiMASK_BITS(stmax) / 2U)) - 1)
/* awful: must constant-evaluate if ut is a signed type */
#define mbi_halftype(ut)	(!mbiTYPE_ISU(ut) ? 07 : \
	((ut)(mbiOshl(ut, 1U, mbiTYPE_UBITS(ut) / 2U) - 1U)))

/* 1. for unsigned types checking afterwards is OK */
#define mbiCAUinc(vl)		mbiCAUadd(vl, 1U)
#define mbiCAUdec(vl)		mbiCAUsub(vl, 1U)
#define mbiCAUadd(vl,vr)	do {					\
	(vl) += mbiUI(vr);						\
	if (__predict_false(mbiUI(vl) < mbiUI(vr)))			\
		mbiCfail;						\
} while (/* CONSTCOND */ 0)
#define mbiCAUsub(vl,vr)	do {					\
	if (__predict_false(mbiUI(vl) < mbiUI(vr)))			\
		mbiCfail;						\
	(vl) -= mbiUI(vr);						\
} while (/* CONSTCOND */ 0)
#define mbiCAUmul(ut,vl,vr)	do {					\
	if (__predict_false(((ut)(vl) > mbi_halftype(ut) ||		\
	    (ut)(vr) > mbi_halftype(ut)) && (vr) != 0 &&		\
	    mbiOU(ut, mbiTYPE_UMAX(ut), /, (ut)(vr)) < (ut)(vl)))	\
		mbiCfail;						\
	(vl) *= (vr);							\
} while (/* CONSTCOND */ 0)

/* 2. for signed types; lim is the prefix (e.g. SHRT) */
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
/* CAP means vr is positive */
#define mbiCAPadd(lim,vl,vr)	do {					\
	if (__predict_false((vl) > (lim ## _MAX - (vr))))		\
		mbiCfail;						\
	(vl) += (vr);							\
} while (/* CONSTCOND */ 0)
#define mbiCAPsub(lim,vl,vr)	do {					\
	if (__predict_false((vl) < (lim ## _MIN + (vr))))		\
		mbiCfail;						\
	(vl) -= (vr);							\
} while (/* CONSTCOND */ 0)
#define mbiCAPmul(lim,vl,vr)	do {					\
	if (__predict_false(((vl) < 0 ||				\
	    (vl) > mbi_halftmax(lim ## _MAX) ||				\
	    (vr) > mbi_halftmax(lim ## _MAX)) &&			\
	    (vr) != 0 && ((vl) < 0 ?					\
	    (vl) < (lim ## _MIN / (vr)) :				\
	    (vl) > (lim ## _MAX / (vr)))))				\
		mbiCfail;						\
	(vl) *= (vr);							\
} while (/* CONSTCOND */ 0)
/* CAS for add/sub/mul may have negative vr */
#define mbiCASadd(lim,vl,vr)	do {					\
	if (__predict_false((vr) < 0 ?					\
	    (vl) < (lim ## _MIN - (vr)) :				\
	    (vl) > (lim ## _MAX - (vr))))				\
		mbiCfail;						\
	(vl) += (vr);							\
} while (/* CONSTCOND */ 0)
#define mbiCASsub(lim,vl,vr)	do {					\
	if (__predict_false((vr) < 0 ?					\
	    (vl) > (lim ## _MAX + (vr)) :				\
	    (vl) < (lim ## _MIN + (vr))))				\
		mbiCfail;						\
	(vl) -= (vr);							\
} while (/* CONSTCOND */ 0)
#define mbiCASmul(lim,vl,vr)	do {					\
	if (__predict_true((vr) >= 0) ? __predict_false(((vl) < 0 ||	\
	    (vl) > mbi_halftmax(lim ## _MAX) ||				\
	    (vr) > mbi_halftmax(lim ## _MAX)) &&			\
	    (vr) != 0 && ((vl) < 0 ?					\
	    (vl) < (lim ## _MIN / (vr)) :				\
	    (vl) > (lim ## _MAX / (vr)))) :				\
	    /* vr < 0 */ __predict_false((vl) < 0 ?			\
	    (vl) < (lim ## _MAX / (vr)) : /* vl >= 0, vr < 0 */		\
	    (vr) != -1 && (vl) > (lim ## _MIN / (vr))))			\
		mbiCfail;						\
	(vl) *= (vr);							\
} while (/* CONSTCOND */ 0)

/* 3. autotype or signed narrowing assign; this is checked IB */
#define mbiCAAlet(vl,srctype,vr) do {					\
	(vl) = (vr);							\
	if (__predict_false((srctype)(vl) != (srctype)(vr)))		\
		mbiCfail;						\
} while (/* CONSTCOND */ 0)
#define mbiCASlet(dsttype,vl,srctype,vr) do {				\
	(vl) = (dsttype)(vr);						\
	if (__predict_false((srctype)(vl) != (srctype)(vr)))		\
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
#define mbiK_signbit(ut,HM)	mbiOU(ut, (HM), +, 1)
#define mbiK_signflip(ut,HM,v)	mbiOU(ut, (v), ^, mbiK_signbit(ut, (HM)))
#define mbiKcmp(ut,HM,vl,op,vr)	mbiCOU(ut, \
		mbiK_signflip(ut, (HM), (vl)), \
		op, \
		mbiK_signflip(ut, (HM), (vr)))
#define mbiMKcmp(ut,FM,HM,vl,op,vr) \
	mbiKcmp(ut, (HM), mbiMM(ut, (FM), (vl)), op, mbiMM(ut, (FM), (vr)))

/* rotate and shift */
#define mbiKrol(ut,vl,vr)	mbiK_sr(ut, mbiK_rol, (vl), (vr), void)
#define mbiKror(ut,vl,vr)	mbiK_sr(ut, mbiK_ror, (vl), (vr), void)
#define mbiKshl(ut,vl,vr)	mbiK_sr(ut, mbiK_shl, (vl), (vr), void)
/* let vz be sgn(vl) */
#define mbiKsar(ut,vz,vl,vr)	mbiK_sr(ut, mbiK_sar, (vl), (vr), (vz))
#define mbiKshr(ut,vl,vr)	mbiK_sr(ut, mbiK_shr, (vl), (vr), 0)
#define mbiMKrol(ut,FM,vl,vr)	mbiMK_sr(ut, (FM), mbiK_rol, (vl), (vr), void)
#define mbiMKror(ut,FM,vl,vr)	mbiMK_sr(ut, (FM), mbiK_ror, (vl), (vr), void)
#define mbiMKshl(ut,FM,vl,vr)	mbiMK_sr(ut, (FM), mbiK_shl, (vl), (vr), void)
#define mbiMKsar(ut,FM,vz,l,r)	mbiMOT(ut, (FM), (vz), \
					mbiK_SR(ut, mbiMASK_BITS(FM), \
					    mbiMK_nr, (l), (r), (FM)), \
					mbiK_SR(ut, mbiMASK_BITS(FM), \
					    mbiK_shr, (l), (r), 0))
#define mbiMKshr(ut,FM,vl,vr)	mbiMK_sr(ut, (FM), mbiK_shr, (vl), (vr), 0)
/* implementation */
#define mbiK_sr(ut,n,vl,vr,vz)	mbiK_SR(ut, mbiTYPE_UBITS(ut), n, vl, vr, vz)
#define mbiMK_sr(ut,FM,n,l,r,z)	mbiMM(ut, (FM), mbiK_SR(ut, mbiMASK_BITS(FM), \
				    n, mbiMM(ut, (FM), (l)), (r), (z)))
#define mbiK_SR(ut,b,n,l,r,vz)	mbiK_RS(ut, b, n, l, mbiUI(r) % mbiUI(b), (vz))
#define mbiK_RS(ut,b,n,v,cl,zx)	mbiOT(ut, cl, n(ut, v, cl, b - (cl), zx), v)
#define mbiK_shl(ut,ax,cl,CL,z)	mbiOshl(ut, ax, cl)
#define mbiK_shr(ut,ax,cl,CL,z)	mbiOshr(ut, ax, cl)
#define mbiK_sar(ut,ax,cl,CL,z)	mbiOT(ut, z, mbiOU1(ut, ~, mbiOshr(ut, \
				    mbiOU1(ut, ~, ax), cl)), \
				    mbiOshr(ut, ax, cl))
#define mbiMK_nr(ut,ax,cl,CL,m)	mbiOU1(ut, ~, mbiOshr(ut, \
				    mbiMO1(ut, m, ~, ax), cl))
#define mbiK_rol(ut,ax,cl,CL,z)	\
	mbiOU(ut, mbiOshl(ut, ax, cl), |, mbiOshr(ut, ax, CL))
#define mbiK_ror(ut,ax,cl,CL,z)	\
	mbiOU(ut, mbiOshr(ut, ax, cl), |, mbiOshl(ut, ax, CL))

/* division and remainder */
#define mbiKdiv(ut,SM,vl,vr)		mbiA_VZU2U(ut, \
	mbiA_U2VZ(ut, (SM), (vl)) ^ mbiA_U2VZ(ut, (SM), (vr)), \
	mbiUI(mbiA_U2M(ut, (SM), (vl))) / mbiUI(mbiA_U2M(ut, (SM), (vr))))
#define mbiMK_div(ut,FM,HM,vl,vr)	mbiA_VZU2U(ut, \
	mbiMA_U2VZ(ut, (FM), (HM), (vl)) ^ mbiMA_U2VZ(ut, (FM), (HM), (vr)), \
	mbiUI(mbiMA_U2M(ut, (FM), (HM), (vl))) / mbiUI(mbiMA_U2M(ut, (FM), (HM), (vr))))
#define mbiMKdiv(ut,FM,HM,vl,vr)	mbiMM(ut, (FM), \
	mbiMK_div(ut, (FM), (HM), (vl), (vr)))
#define mbiK_rem(ut,vl,vr,vdiv)		mbiOU(ut, vl, -, mbiOU(ut, vdiv, *, vr))
#define mbiKrem(ut,SM,vl,vr)		mbiK_rem(ut, (vl), (vr), \
					    mbiKdiv(ut, (SM), (vl), (vr)))
#define mbiMKrem(ut,FM,HM,vl,vr)	mbiMM(ut, (FM), mbiK_rem(ut, (vl), (vr), \
					    mbiMK_div(ut, (FM), (HM), (vl), (vr))))
/* statement, assigning to dstdiv and dstrem */
#define mbiKdivrem(dstdiv,dstrem,ut,SM,vl,vr) do {			\
	ut mbi__TMP = mbiKdiv(ut, (SM), (vl), (vr));			\
	(dstdiv) = mbi__TMP;						\
	(dstrem) = mbiK_rem(ut, (vl), (vr), mbi__TMP);			\
} while (/* CONSTCOND */ 0)
#define mbiMKdivrem(dstdiv,dstrem,ut,FM,HM,vl,vr) do {			\
	ut mbi__TMP = mbiMK_div(ut, (FM), (HM), (vl), (vr));		\
	(dstdiv) = mbiMM(ut, (FM), mbi__TMP);				\
	(dstrem) = mbiMM(ut, (FM), mbiK_rem(ut, (vl), (vr), mbi__TMP));	\
} while (/* CONSTCOND */ 0)

/* nil pointer constant */
#if (defined(__cplusplus) && (__cplusplus >= 201103L)) || \
    (defined(__STDC_VERSION__) && ((__STDC_VERSION__) >= 202311L))
#define mbi_nil				nullptr
#else
#define mbi_nil				(void *)0UL
#endif

#endif /* !SYSKERN_MBSDINT_H */
