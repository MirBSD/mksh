/*-
 * MirBSD sanitisation attempt for C integer madness
 *
 * © mirabilos Ⓕ CC0 or The MirOS Licence (MirBSD)
 */

#ifndef SYSKERN_MBSDINT_H
#define SYSKERN_MBSDINT_H "$MirOS: src/bin/mksh/mbsdint.h,v 1.61 2024/07/25 02:29:43 tg Exp $"

/*
 * cpp defines to set:
 *
 * -DHAVE_INTCONSTEXPR_RSIZE_MAX=1 if RSIZE_MAX is an integer constexpr
 * -DHAVE_OFF_T=0 if you do not supply the POSIX off_t typedef
 *
 * -DMBSDINT_H_SMALL_SYSTEM=1 (or 2) to shorten some macros, for old systems
 * or -DMBSDINT_H_SMALL_SYSTEM=3 in very tiny systems, e.g. 16-bit
 *
 * -DMBSDINT_H_MBIPTR_IS_SIZET=0 stop assuming mbiPTR is size_t-ranged
 * -DMBSDINT_H_MBIPTR_IN_LARGE=0 mbiPTR fits mbiHUGE but no longer mbiLARGE
 * -DMBSDINT_H_WANT_LONG_IN_SIZET=0 don’t ensure long fits in size_t (16-bit)
 *
 * -DMBSDINT_H_WANT_PTR_IN_SIZET (breaks some) ensure pointers fit in size_t
 * -DMBSDINT_H_WANT_SIZET_IN_LONG (breaks LLP64) ensure size_t fits in long
 * -DMBSDINT_H_WANT_INT32=1 to ensure unsigned int has at least 32 bit width
 * -DMBSDINT_H_WANT_LRG64=1 to ensure mbiLARGE_U has at least 64 bit width
 * -DMBSDINT_H_WANT_SAFEC=1 to ensure mbiSAFECOMPLEMENT==1
 * (these trigger extra compile-time assertion checks)
 *
 * This header always includes <limits.h> so please ensure any possible
 * prerequisites as well as all existing of the following headers…
 *	<sys/types.h>		// from POSIX
 *	<basetsd.h>		// on Windows®
 *	<inttypes.h>		// some pre-C99
 *	<stdint.h>		// ISO C99
 *	"mbsdcc.h"		// from this directory, in this order
 * … are included before this header, for full functionality.
 */

#if !defined(_KERNEL) && !defined(_STANDALONE)
#include <limits.h>
#else
#include <sys/cdefs.h>
#include <sys/limits.h>
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4127 4146 4296)
#endif

#if !defined(MBSDINT_H_SMALL_SYSTEM) || ((MBSDINT_H_SMALL_SYSTEM) < 1)
#undef MBSDINT_H_SMALL_SYSTEM	/* in case someone defined it as 0 */
#define mbiMASK_bitmax		279
#elif (MBSDINT_H_SMALL_SYSTEM) >= 2
#define mbiMASK_bitmax		75
#else
#define mbiMASK_bitmax		93
#endif

#if defined(HAVE_INTCONSTEXPR_RSIZE_MAX) && ((HAVE_INTCONSTEXPR_RSIZE_MAX) < 1)
#undef HAVE_INTCONSTEXPR_RSIZE_MAX
#endif
#if defined(MBSDINT_H_WANT_PTR_IN_SIZET) && ((MBSDINT_H_WANT_PTR_IN_SIZET) < 1)
#undef MBSDINT_H_WANT_PTR_IN_SIZET
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
#if defined(MBSDINT_H_WANT_SAFEC) && ((MBSDINT_H_WANT_SAFEC) < 1)
#undef MBSDINT_H_WANT_SAFEC
#endif

#ifndef HAVE_OFF_T
#define HAVE_OFF_T 1
#endif
#ifndef MBSDINT_H_MBIPTR_IS_SIZET
#define MBSDINT_H_MBIPTR_IS_SIZET 1
#endif
#ifndef MBSDINT_H_MBIPTR_IN_LARGE
#define MBSDINT_H_MBIPTR_IN_LARGE 1
#endif
#ifndef MBSDINT_H_WANT_LONG_IN_SIZET
#define MBSDINT_H_WANT_LONG_IN_SIZET 1
#endif

/* expose SIZE_MAX if possible and provided, not guessed */
#ifndef SIZE_MAX
#if defined(SIZE_T_MAX)
#define SIZE_MAX		SIZE_T_MAX
#elif defined(MAXSIZE_T)
#define SIZE_MAX		MAXSIZE_T
#endif
#endif

/* expose POSIX SSIZE_MAX and ssize_t on Win32 */
#if defined(SIZE_MAX) && !defined(SSIZE_MAX) && \
    defined(MINSSIZE_T) && defined(MAXSSIZE_T) && !defined(ssize_t)
#define SSIZE_MIN		MINSSIZE_T
#define SSIZE_MAX		MAXSSIZE_T
#define ssize_t			SSIZE_T
#endif

/* special compile-time assertions */
#define mbiCTA_TYPE_MBIT(nm,ty)	mbccCTA(ctatm_ ## nm, \
	(sizeof(ty) <= (mbiMASK_bitmax / (CHAR_BIT))))

/* kinds of types */
	/* compile-time and runtime */
#define mbiTYPE_ISF(type)	(!!(0+(int)(2 * (type)0.5)))
#define mbiTYPE_ISU(type)	((type)-1 > (type)0)
/* limits of types */
#define mbiTYPE_UMAX(type)	((type)~(type)0U)
#define mbiTYPE_UBITS(type)	mbiMASK_BITS(mbiTYPE_UMAX(type))
/* calculation by Hallvard B Furuseth (via comp.lang.c), ≤ 2039 bit */
#define mbiMASK__lh(maxv)	((maxv) / ((maxv) % 255 + 1) / 255 % 255 * 8)
#define mbiMASK__rh(maxv)	(7 - 86 / ((maxv) % 255 + 12))
/* mbiMASK_BITS everywhere except #if uses (castless) mbiMASK__BITS */
#define mbiMASK__BITS(maxv)	(mbiMASK__lh(maxv) + mbiMASK__rh(maxv))
#define mbiMASK__type(maxv)	(mbiMASK__lh(maxv) + (int)mbiMASK__rh(maxv))
#define mbiMASK_BITS(maxv)	(0U + (unsigned int)mbiMASK__type(maxv))

/* ensure v is a positive (2ⁿ-1) number (n>0), up to 279 (or less) bits */
#define mbiMASK_CHK(v)		mbmscWd(4296) \
				((v) > 0 ? mbi_maskchk31_1((v)) : 0) \
				mbmscWpop
#define mbi_maskchks(v,m,o,n)	(v <= m ? o : ((v & m) == m) && n)
#define mbi_maskchk31s(v,n)	mbi_maskchks(v, 0x7FFFFFFFUL, \
				    mbi_maskchk16(v), n)
#define mbi_maskchk15s(v,n)	mbi_maskchks(v, 0x7FFFUL, \
				    mbi_maskchk8(v), n)
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
#define mbi_maskchk31_1(v)	mbi_maskchk15s(v, mbi_maskchk15_2(v >> 15))
#define mbi_maskchk15_2(v)	mbi_maskchk15s(v, mbi_maskchk15_3(v >> 15))
#define mbi_maskchk15_3(v)	mbi_maskchk15s(v, mbi_maskchk15_4(v >> 15))
#define mbi_maskchk15_4(v)	mbi_maskchk15s(v, mbi_maskchk15_9(v >> 15))
#define mbi_maskchk15_9(v)	(v <= 0x7FFFUL && mbi_maskchk8(v))
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
#if defined(HAVE_INTCONSTEXPR_RSIZE_MAX)
/* trust but verify operating environment */
#define mbiRSZCHK	RSIZE_MAX
#define mbiSIZE_MAX	((size_t)RSIZE_MAX)
#elif !defined(SIZE_MAX)
#ifdef PTRDIFF_MAX
#define mbiRSZCHK	PTRDIFF_MAX
#define mbiSIZE_MAX	((size_t)PTRDIFF_MAX)
#elif defined(MBSDINT_H_SMALL_SYSTEM) && ((MBSDINT_H_SMALL_SYSTEM) >= 3)
#define mbiSIZE_MAX	mbiTYPE_UMAX(size_t)
#else
#define mbiSIZE_MAX	((size_t)(mbiTYPE_UMAX(size_t) >> 1))
#endif /* !PTRDIFF_MAX !MBSDINT_H_SMALL_SYSTEM>=3 */
#elif !defined(PTRDIFF_MAX) || \
    (((SIZE_MAX) == (PTRDIFF_MAX)) && ((SIZE_MAX) > 0x7FFFFFFFUL))
#if defined(MBSDINT_H_SMALL_SYSTEM) && ((MBSDINT_H_SMALL_SYSTEM) >= 3)
#define mbiSIZE_MAX	((size_t)SIZE_MAX)
#else
#define mbiSIZE_MAX	((size_t)(((size_t)SIZE_MAX) >> 1))
#endif
#elif (SIZE_MAX) < (PTRDIFF_MAX)
#define mbiSIZE_MAX	((size_t)SIZE_MAX)
#else
#define mbiRSZCHK	PTRDIFF_MAX
#define mbiSIZE_MAX	((size_t)PTRDIFF_MAX)
#endif
/* size_t must fit %lu/%llu if there is no %zu support yet */
#if (defined(__STDC_VERSION__) && ((__STDC_VERSION__) >= 199901L)) || \
    (defined(__cplusplus) && ((__cplusplus) >= 201103L))
#define mbiSIZE_S	ssize_t
#define mbiSIZE_U	size_t
#define mbiSIZE_P(c)	"z" #c
#elif defined(ULLONG_MAX) && defined(SIZE_MAX) && ((SIZE_MAX) > (ULONG_MAX))
#define mbiSIZE_S	long long
#define mbiSIZE_U	unsigned long long
#define mbiSIZE_P(c)	"ll" #c
#else
#define mbiSIZE_S	long
#define mbiSIZE_U	unsigned long
#define mbiSIZE_P(c)	"l" #c
#endif
#define mbiSIZE_PV(v)	((mbiSIZE_U)(v))
#ifndef SSIZE_MAX
#undef mbiSIZE_S
#endif

/* update mbsdcc.h definition */
#undef mbccSIZE_MAX
#define mbccSIZE_MAX	mbiSIZE_MAX

/* largest integer (need not fit a full wide pointer, e.g. on CHERI) */
#if defined(INTMAX_MIN)
#define mbiHUGE_S		intmax_t
#define mbiHUGE_S_MIN		INTMAX_MIN
#define mbiHUGE_S_MAX		INTMAX_MAX
#define mbiHUGE_U		uintmax_t
#define mbiHUGE_U_MAX		UINTMAX_MAX
#define mbiHUGE_P(c)		PRI ## c ## MAX
#elif defined(LLONG_MIN)
#define mbiHUGE_S		long long
#define mbiHUGE_S_MIN		LLONG_MIN
#define mbiHUGE_S_MAX		LLONG_MAX
#define mbiHUGE_U		unsigned long long
#define mbiHUGE_U_MAX		ULLONG_MAX
#define mbiHUGE_P(c)		"ll" #c
#elif defined(_UI64_MAX) && (mbiMASK__BITS(_UI64_MAX) > mbiMASK__BITS(ULONG_MAX))
#define mbiHUGE_S		signed __int64
#define mbiHUGE_S_MIN		_I64_MIN
#define mbiHUGE_S_MAX		_I64_MAX
#define mbiHUGE_U		unsigned __int64
#define mbiHUGE_U_MAX		_UI64_MAX
#define mbiHUGE_UBITS		64
#define mbiHUGE_P(c)		"I64" #c
#elif defined(QUAD_MIN)
#define mbiHUGE_S		quad_t
#define mbiHUGE_S_MIN		QUAD_MIN
#define mbiHUGE_S_MAX		QUAD_MAX
#define mbiHUGE_U		u_quad_t
#define mbiHUGE_U_MAX		UQUAD_MAX
#define mbiHUGE_UBITS		64
#define mbiHUGE_P(c)		"q" #c
#else /* C89? */
#define mbiHUGE_S		long
#define mbiHUGE_S_MIN		LONG_MIN
#define mbiHUGE_S_MAX		LONG_MAX
#define mbiHUGE_U		unsigned long
#define mbiHUGE_U_MAX		ULONG_MAX
#define mbiHUGE_P(c)		"l" #c
#endif
/* larger than int, 64 bit if possible; storage fits off_t, size_t */
#if (mbiMASK__BITS(ULONG_MAX) >= 64)
#define mbiLARGE_S		long
#define mbiLARGE_S_MIN		LONG_MIN
#define mbiLARGE_S_MAX		LONG_MAX
#define mbiLARGE_U		unsigned long
#define mbiLARGE_U_MAX		ULONG_MAX
#define mbiLARGE_P(c)		"l" #c
#elif defined(ULLONG_MAX) && (mbiMASK__BITS(ULLONG_MAX) >= 64)
#define mbiLARGE_S		long long
#define mbiLARGE_S_MIN		LLONG_MIN
#define mbiLARGE_S_MAX		LLONG_MAX
#define mbiLARGE_U		unsigned long long
#define mbiLARGE_U_MAX		ULLONG_MAX
#define mbiLARGE_P(c)		"ll" #c
#elif defined(_UI64_MAX) && (mbiMASK__BITS(_UI64_MAX) >= 64)
#define mbiLARGE_S		signed __int64
#define mbiLARGE_S_MIN		_I64_MIN
#define mbiLARGE_S_MAX		_I64_MAX
#define mbiLARGE_U		unsigned __int64
#define mbiLARGE_U_MAX		_UI64_MAX
#define mbiLARGE_UBITS		64
#define mbiLARGE_P(c)		"I64" #c
#elif defined(UQUAD_MAX)
#define mbiLARGE_S		quad_t
#define mbiLARGE_S_MIN		QUAD_MIN
#define mbiLARGE_S_MAX		QUAD_MAX
#define mbiLARGE_U		u_quad_t
#define mbiLARGE_U_MAX		UQUAD_MAX
#define mbiLARGE_UBITS		64
#define mbiLARGE_P(c)		"q" #c
#else
#define mbiLARGE_S		mbiHUGE_S
#define mbiLARGE_S_MIN		mbiHUGE_S_MIN
#define mbiLARGE_S_MAX		mbiHUGE_S_MAX
#define mbiLARGE_U		mbiHUGE_U
#define mbiLARGE_U_MAX		mbiHUGE_U_MAX
#define mbiLARGE_P		mbiHUGE_P
#endif
/* integer type that can keep the base address of a pointer */
#ifdef __CHERI__
#define mbiPTR_U		ptraddr_t
#define mbiPTR_U_MAX		mbiTYPE_UMAX(mbiPTR_U)
#elif defined(UINTPTR_MAX)
#define mbiPTR_U		uintptr_t
#define mbiPTR_U_MAX		UINTPTR_MAX
#else
#define mbiPTR_U		size_t
#define mbiPTR_U_MAX		mbiTYPE_UMAX(mbiPTR_U)
#endif
#if MBSDINT_H_MBIPTR_IS_SIZET || \
    (!defined(__CHERI__) && !defined(UINTPTR_MAX))
#define mbiPTR_P		mbiSIZE_P
#define mbiPTR_PV(v)		((mbiSIZE_U)(v))
#elif MBSDINT_H_MBIPTR_IN_LARGE
#define mbiPTR_P		mbiLARGE_P
#define mbiPTR_PV(v)		((mbiLARGE_U)(v))
#else
#define mbiPTR_P		mbiHUGE_P
#define mbiPTR_PV(v)		((mbiHUGE_U)(v))
#endif

#undef mbiSAFECOMPLEMENT
#if ((SCHAR_MIN) == -(SCHAR_MAX))
#define mbiSAFECOMPLEMENT 0
#elif ((SCHAR_MIN)+1 == -(SCHAR_MAX))
#define mbiSAFECOMPLEMENT 1
#endif

#ifndef MBSDINT_H_SKIP_CTAS
/* compile-time assertions for mbsdint.h */
mbCTA_BEG(mbsdint_h);
 /* compiler (in)sanity, from autoconf */
#define mbiCTf(x) 'x'
 mbCTA(xlc6, mbiCTf(a) == 'x');
#undef mbiCTf
 mbCTA(osf4, '\x00' == 0);
 /* C types */
 mbCTA(basic_char_smask, mbiMASK_CHK(SCHAR_MAX));
 mbCTA(basic_char_umask, mbiMASK_CHK(UCHAR_MAX));
 mbCTA(basic_char,
	sizeof(char) == 1 && (CHAR_BIT) >= 7 && (CHAR_BIT) < 2040 &&
	(((CHAR_MAX) == (SCHAR_MAX) && (CHAR_MIN) == (SCHAR_MIN)) ||
	 ((CHAR_MAX) == (UCHAR_MAX) && (CHAR_MIN) == 0)) &&
	mbiTYPE_UMAX(unsigned char) == (UCHAR_MAX) &&
	sizeof(signed char) == 1 &&
	sizeof(unsigned char) == 1 &&
	mbiTYPE_UBITS(unsigned char) >= 8 && (SCHAR_MIN) < 0 &&
	((SCHAR_MIN) == -(SCHAR_MAX) || (SCHAR_MIN)+1 == -(SCHAR_MAX)) &&
	mbiTYPE_UBITS(unsigned char) == (unsigned int)(CHAR_BIT));
#ifndef mbiSAFECOMPLEMENT
 mbCTA(basic_char_complement, /* fail */ 0);
#endif
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
#ifdef QUAD_MIN
 mbiCTA_TYPE_MBIT(ucb_sq, quad_t);
 mbiCTA_TYPE_MBIT(ucb_uq, u_quad_t);
#endif
 mbiCTA_TYPE_MBIT(shuge, mbiHUGE_S);
 mbiCTA_TYPE_MBIT(uhuge, mbiHUGE_U);
 mbiCTA_TYPE_MBIT(slarge, mbiLARGE_S);
 mbiCTA_TYPE_MBIT(ularge, mbiLARGE_U);
 mbiCTA_TYPE_MBIT(uptr, mbiPTR_U);
 mbCTA(basic_short_smask, mbiMASK_CHK(SHRT_MAX));
 mbCTA(basic_short_umask, mbiMASK_CHK(USHRT_MAX));
 mbCTA(basic_short,
	mbiTYPE_UMAX(unsigned short) == (USHRT_MAX) &&
	mbiTYPE_UBITS(unsigned short) >= 16 && (SHRT_MIN) < 0 &&
	((SHRT_MIN) == -(SHRT_MAX) || (SHRT_MIN)+1 == -(SHRT_MAX)) &&
	((SHRT_MIN) == -(SHRT_MAX)) == ((SCHAR_MIN) == -(SCHAR_MAX)) &&
	sizeof(short) >= sizeof(signed char) &&
	sizeof(unsigned short) >= sizeof(unsigned char) &&
	mbiMASK_BITS(SHRT_MAX) >= mbiMASK_BITS(SCHAR_MAX) &&
	mbiMASK_BITS(SHRT_MAX) < mbiMASK_BITS(mbiHUGE_S_MAX) &&
	mbiMASK_BITS(USHRT_MAX) >= mbiMASK_BITS(UCHAR_MAX) &&
	mbiMASK_BITS(USHRT_MAX) < mbiMASK_BITS(mbiHUGE_U_MAX) &&
	sizeof(short) == sizeof(unsigned short));
 mbCTA(basic_int_smask, mbiMASK_CHK(INT_MAX));
 mbCTA(basic_int_umask, mbiMASK_CHK(UINT_MAX));
 mbCTA(basic_int,
	mbiTYPE_UMAX(unsigned int) == (UINT_MAX) &&
	mbiTYPE_UBITS(unsigned int) >= 16 && (INT_MIN) < 0 &&
	((INT_MIN) == -(INT_MAX) || (INT_MIN)+1 == -(INT_MAX)) &&
	((INT_MIN) == -(INT_MAX)) == ((SCHAR_MIN) == -(SCHAR_MAX)) &&
	sizeof(int) >= sizeof(short) &&
	sizeof(unsigned int) >= sizeof(unsigned short) &&
	mbiMASK_BITS(INT_MAX) >= mbiMASK_BITS(SHRT_MAX) &&
	mbiMASK_BITS(INT_MAX) <= mbiMASK_BITS(mbiHUGE_S_MAX) &&
	mbiMASK_BITS(UINT_MAX) >= mbiMASK_BITS(USHRT_MAX) &&
	mbiMASK_BITS(UINT_MAX) <= mbiMASK_BITS(mbiHUGE_U_MAX) &&
	sizeof(int) == sizeof(unsigned int));
 mbCTA(basic_long_smask, mbiMASK_CHK(LONG_MAX));
 mbCTA(basic_long_umask, mbiMASK_CHK(ULONG_MAX));
 mbCTA(basic_long,
	mbiTYPE_UMAX(unsigned long) == (ULONG_MAX) &&
	mbiTYPE_UBITS(unsigned long) >= 32 && (LONG_MIN) < 0 &&
	((LONG_MIN) == -(LONG_MAX) || (LONG_MIN)+1 == -(LONG_MAX)) &&
	((LONG_MIN) == -(LONG_MAX)) == ((SCHAR_MIN) == -(SCHAR_MAX)) &&
	sizeof(long) >= sizeof(int) &&
	sizeof(unsigned long) >= sizeof(unsigned int) &&
	mbiMASK_BITS(LONG_MAX) >= mbiMASK_BITS(INT_MAX) &&
	mbiMASK_BITS(LONG_MAX) <= mbiMASK_BITS(mbiHUGE_S_MAX) &&
	mbiMASK_BITS(ULONG_MAX) >= mbiMASK_BITS(UINT_MAX) &&
	mbiMASK_BITS(ULONG_MAX) <= mbiMASK_BITS(mbiHUGE_U_MAX) &&
	sizeof(long) == sizeof(unsigned long));
 /* __int64 is MSC */
#ifdef _UI64_MAX
 mbCTA(basic_i64_smask, mbiMASK_CHK(_I64_MAX));
 mbCTA(basic_i64_umask, mbiMASK_CHK(_UI64_MAX));
 mbCTA(basic_i64,
	!mbiTYPE_ISF(unsigned __int64) && !mbiTYPE_ISF(signed __int64) &&
	mbiTYPE_ISU(unsigned __int64) && !mbiTYPE_ISU(signed __int64) &&
	mbiTYPE_UMAX(unsigned __int64) == (_UI64_MAX) &&
	mbiTYPE_UBITS(unsigned __int64) == 64 && (_I64_MIN) < 0 &&
	((_I64_MIN)+1 == -(_I64_MAX)) &&
	((_I64_MIN) == -(_I64_MAX)) == ((SCHAR_MIN) == -(SCHAR_MAX)) &&
	sizeof(signed __int64) >= sizeof(long) &&
	sizeof(unsigned __int64) >= sizeof(unsigned long) &&
	sizeof(signed __int64) <= sizeof(mbiHUGE_S) &&
	sizeof(unsigned __int64) <= sizeof(mbiHUGE_U) &&
	mbiMASK_BITS(_I64_MAX) >= mbiMASK_BITS(LONG_MAX) &&
	mbiMASK_BITS(_I64_MAX) <= mbiMASK_BITS(mbiHUGE_S_MAX) &&
	mbiMASK_BITS(_UI64_MAX) >= mbiMASK_BITS(ULONG_MAX) &&
	mbiMASK_BITS(_UI64_MAX) <= mbiMASK_BITS(mbiHUGE_U_MAX) &&
	sizeof(signed __int64) == sizeof(unsigned __int64));
#endif
 /* legacy 4.4BSD extension, not cpp-safe, no suffixed constants */
#ifdef QUAD_MIN
 mbCTA(basic_quad_smask, mbiMASK_CHK(QUAD_MAX));
 mbCTA(basic_quad_umask, mbiMASK_CHK(UQUAD_MAX));
 mbCTA(basic_quad,
	!mbiTYPE_ISF(u_quad_t) && !mbiTYPE_ISF(quad_t) &&
	mbiTYPE_ISU(u_quad_t) && !mbiTYPE_ISU(quad_t) &&
	mbiTYPE_UMAX(u_quad_t) == (UQUAD_MAX) &&
	mbiTYPE_UBITS(u_quad_t) == 64 && (QUAD_MIN) < 0 &&
	((QUAD_MIN)+1 == -(QUAD_MAX)) &&
	((QUAD_MIN) == -(QUAD_MAX)) == ((SCHAR_MIN) == -(SCHAR_MAX)) &&
	sizeof(quad_t) >= sizeof(long) &&
	sizeof(u_quad_t) >= sizeof(unsigned long) &&
	sizeof(quad_t) <= sizeof(mbiHUGE_S) &&
	sizeof(u_quad_t) <= sizeof(mbiHUGE_U) &&
	mbiMASK_BITS(QUAD_MAX) >= mbiMASK_BITS(LONG_MAX) &&
	mbiMASK_BITS(QUAD_MAX) <= mbiMASK_BITS(mbiHUGE_S_MAX) &&
	mbiMASK_BITS(UQUAD_MAX) >= mbiMASK_BITS(ULONG_MAX) &&
	mbiMASK_BITS(UQUAD_MAX) <= mbiMASK_BITS(mbiHUGE_U_MAX) &&
	sizeof(quad_t) == sizeof(u_quad_t));
#endif
 /* common pre-C99 extension now standardised */
#ifdef LLONG_MIN
 mbCTA(basic_llong_smask, mbiMASK_CHK(LLONG_MAX));
 mbCTA(basic_llong_umask, mbiMASK_CHK(ULLONG_MAX));
 mbCTA(basic_llong,
	mbiTYPE_UMAX(unsigned long long) == (ULLONG_MAX) &&
	mbiTYPE_UBITS(unsigned long long) >= 64 && (LLONG_MIN) < 0 &&
	((LLONG_MIN) == -(LLONG_MAX) || (LLONG_MIN)+1 == -(LLONG_MAX)) &&
	((LLONG_MIN) == -(LLONG_MAX)) == ((SCHAR_MIN) == -(SCHAR_MAX)) &&
	sizeof(long long) >= sizeof(long) &&
	sizeof(unsigned long long) >= sizeof(unsigned long) &&
	sizeof(long long) <= sizeof(mbiHUGE_S) &&
	sizeof(unsigned long long) <= sizeof(mbiHUGE_U) &&
	mbiMASK_BITS(LLONG_MAX) >= mbiMASK_BITS(LONG_MAX) &&
	mbiMASK_BITS(LLONG_MAX) <= mbiMASK_BITS(mbiHUGE_S_MAX) &&
	mbiMASK_BITS(ULLONG_MAX) >= mbiMASK_BITS(ULONG_MAX) &&
	mbiMASK_BITS(ULLONG_MAX) <= mbiMASK_BITS(mbiHUGE_U_MAX) &&
	sizeof(long long) == sizeof(unsigned long long));
#endif
 /* {,u}intmax_t are C99 */
#ifdef INTMAX_MIN
 mbCTA(basic_imax_smask, mbiMASK_CHK(INTMAX_MAX));
 mbCTA(basic_imax_umask, mbiMASK_CHK(UINTMAX_MAX));
 mbCTA(basic_imax,
	!mbiTYPE_ISF(uintmax_t) && !mbiTYPE_ISF(intmax_t) &&
	mbiTYPE_ISU(uintmax_t) && !mbiTYPE_ISU(intmax_t) &&
	mbiTYPE_UMAX(uintmax_t) == (UINTMAX_MAX) &&
	mbiTYPE_UBITS(uintmax_t) >= 32 && (INTMAX_MIN) < 0 &&
	((INTMAX_MIN) == -(INTMAX_MAX) || (INTMAX_MIN)+1 == -(INTMAX_MAX)) &&
	((INTMAX_MIN) == -(INTMAX_MAX)) == ((SCHAR_MIN) == -(SCHAR_MAX)) &&
	sizeof(intmax_t) >= sizeof(long) &&
	sizeof(uintmax_t) >= sizeof(unsigned long) &&
	sizeof(intmax_t) <= sizeof(mbiHUGE_S) &&
	sizeof(uintmax_t) <= sizeof(mbiHUGE_U) &&
	mbiMASK_BITS(INTMAX_MAX) >= mbiMASK_BITS(LONG_MAX) &&
	mbiMASK_BITS(INTMAX_MAX) <= mbiMASK_BITS(mbiHUGE_S_MAX) &&
	mbiMASK_BITS(UINTMAX_MAX) >= mbiMASK_BITS(ULONG_MAX) &&
	mbiMASK_BITS(UINTMAX_MAX) <= mbiMASK_BITS(mbiHUGE_U_MAX) &&
	sizeof(intmax_t) == sizeof(uintmax_t));
#ifdef LLONG_MIN
 mbCTA(basic_imax_llong,
	mbiMASK_BITS(INTMAX_MAX) >= mbiMASK_BITS(LLONG_MAX) &&
	mbiMASK_BITS(UINTMAX_MAX) >= mbiMASK_BITS(ULLONG_MAX) &&
	sizeof(intmax_t) >= sizeof(long long) &&
	sizeof(uintmax_t) >= sizeof(unsigned long long));
#endif /* INTMAX_MIN && LLONG_MIN */
#ifdef _I64_MIN
 mbCTA(basic_imax_i64,
	mbiMASK_BITS(INTMAX_MAX) >= mbiMASK_BITS(_I64_MAX) &&
	mbiMASK_BITS(UINTMAX_MAX) >= mbiMASK_BITS(_UI64_MAX) &&
	sizeof(intmax_t) >= sizeof(signed __int64) &&
	sizeof(uintmax_t) >= sizeof(unsigned __int64));
#endif /* INTMAX_MIN && _I64_MIN */
#ifdef QUAD_MIN
 mbCTA(basic_imax_quad,
	mbiMASK_BITS(INTMAX_MAX) >= mbiMASK_BITS(QUAD_MAX) &&
	mbiMASK_BITS(UINTMAX_MAX) >= mbiMASK_BITS(UQUAD_MAX) &&
	sizeof(intmax_t) >= sizeof(quad_t) &&
	sizeof(uintmax_t) >= sizeof(u_quad_t));
#endif /* INTMAX_MIN && QUAD_MIN */
#endif /* INTMAX_MIN */
 /* mbi{LARGE,HUGE}_{S,U} are defined by us */
 mbCTA(basic_large_smask, mbiMASK_CHK(mbiLARGE_S_MAX));
 mbCTA(basic_large_umask, mbiMASK_CHK(mbiLARGE_U_MAX));
 mbCTA(basic_large,
	!mbiTYPE_ISF(mbiLARGE_U) && !mbiTYPE_ISF(mbiLARGE_S) &&
	mbiTYPE_ISU(mbiLARGE_U) && !mbiTYPE_ISU(mbiLARGE_S) &&
	mbiTYPE_UMAX(mbiLARGE_U) == (mbiLARGE_U_MAX) && (mbiLARGE_S_MIN) < 0 &&
	((mbiLARGE_S_MIN) == -(mbiLARGE_S_MAX) || (mbiLARGE_S_MIN)+1 == -(mbiLARGE_S_MAX)) &&
	((mbiLARGE_S_MIN) == -(mbiLARGE_S_MAX)) == ((SCHAR_MIN) == -(SCHAR_MAX)) &&
	sizeof(mbiLARGE_S) >= sizeof(long) &&
	sizeof(mbiLARGE_U) >= sizeof(unsigned long) &&
	mbiMASK_BITS(mbiLARGE_S_MAX) >= mbiMASK_BITS(LONG_MAX) &&
	mbiMASK_BITS(mbiLARGE_S_MAX) <= mbiMASK_BITS(mbiHUGE_S_MAX) &&
	mbiMASK_BITS(mbiLARGE_U_MAX) >= mbiMASK_BITS(ULONG_MAX) &&
	mbiMASK_BITS(mbiLARGE_U_MAX) <= mbiMASK_BITS(mbiHUGE_U_MAX) &&
	sizeof(mbiLARGE_S) == sizeof(mbiLARGE_U));
 mbCTA(basic_huge_smask, mbiMASK_CHK(mbiHUGE_S_MAX));
 mbCTA(basic_huge_umask, mbiMASK_CHK(mbiHUGE_U_MAX));
 mbCTA(basic_huge,
	!mbiTYPE_ISF(mbiHUGE_U) && !mbiTYPE_ISF(mbiHUGE_S) &&
	mbiTYPE_ISU(mbiHUGE_U) && !mbiTYPE_ISU(mbiHUGE_S) &&
	mbiTYPE_UMAX(mbiHUGE_U) == (mbiHUGE_U_MAX) && (mbiHUGE_S_MIN) < 0 &&
	((mbiHUGE_S_MIN) == -(mbiHUGE_S_MAX) || (mbiHUGE_S_MIN)+1 == -(mbiHUGE_S_MAX)) &&
	((mbiHUGE_S_MIN) == -(mbiHUGE_S_MAX)) == ((SCHAR_MIN) == -(SCHAR_MAX)) &&
	sizeof(mbiHUGE_S) >= sizeof(mbiLARGE_S) &&
	sizeof(mbiHUGE_U) >= sizeof(mbiLARGE_U) &&
	mbiMASK_BITS(mbiHUGE_S_MAX) >= mbiMASK_BITS(LONG_MAX) &&
	mbiMASK_BITS(mbiHUGE_U_MAX) >= mbiMASK_BITS(ULONG_MAX) &&
	sizeof(mbiHUGE_S) == sizeof(mbiHUGE_U));
 /* off_t is POSIX, with no _MIN/_MAX constants (WTF‽) */
#if HAVE_OFF_T
 mbCTA(basic_offt,
	sizeof(off_t) >= sizeof(int) &&
	sizeof(off_t) <= sizeof(mbiLARGE_S) &&
	!mbiTYPE_ISF(off_t) && !mbiTYPE_ISU(off_t));
#endif
 /* ptrdiff_t is C */
 mbCTA(basic_ptrdifft,
	sizeof(ptrdiff_t) >= sizeof(int) &&
	sizeof(ptrdiff_t) <= sizeof(mbiHUGE_S) &&
	!mbiTYPE_ISF(ptrdiff_t) && !mbiTYPE_ISU(ptrdiff_t));
#ifdef PTRDIFF_MAX
 mbCTA(basic_ptrdifft_mask, mbiMASK_CHK(PTRDIFF_MAX));
 mbCTA(basic_ptrdifft_max,
	mbiMASK_BITS(PTRDIFF_MAX) >= mbiMASK_BITS(INT_MAX) &&
	mbiMASK_BITS(PTRDIFF_MAX) <= mbiMASK_BITS(mbiHUGE_S_MAX) &&
	((mbiHUGE_S)(PTRDIFF_MAX) == (mbiHUGE_S)(ptrdiff_t)(PTRDIFF_MAX)));
#endif
 /* size_t is C */
 mbCTA(basic_sizet,
	sizeof(size_t) >= sizeof(unsigned int) &&
	sizeof(size_t) <= sizeof(mbiHUGE_U) &&
	!mbiTYPE_ISF(size_t) && mbiTYPE_ISU(size_t) &&
	mbiTYPE_UBITS(size_t) >= mbiMASK_BITS(UINT_MAX) &&
	mbiTYPE_UBITS(size_t) <= mbiMASK_BITS(mbiHUGE_U_MAX));
#ifdef SIZE_MAX
 mbCTA(basic_sizet_mask, mbiMASK_CHK(SIZE_MAX));
 mbCTA(basic_sizet_max,
	mbiMASK_BITS(SIZE_MAX) >= mbiMASK_BITS(USHRT_MAX) &&
	mbiMASK_BITS(SIZE_MAX) <= mbiTYPE_UBITS(size_t) &&
	((mbiHUGE_U)(SIZE_MAX) == (mbiHUGE_U)(size_t)(SIZE_MAX)));
#endif
 /* mbiSIZE_S is only defined if ssize_t is defined */
 /* but consistently mapped, no extra checks here */
 mbCTA(basic_sizet_P,
	sizeof(size_t) <= sizeof(mbiSIZE_U) &&
	mbiTYPE_UBITS(size_t) <= mbiTYPE_UBITS(mbiSIZE_U));
 /* note mbiSIZE_MAX is not necessarily a bitmask (0b0*1+) */
#ifdef mbiRSZCHK
 mbCTA(smax_range, (0U + (mbiRSZCHK)) <= (0U + (mbiHUGE_U_MAX)));
 mbCTA(smax_check, ((mbiHUGE_U)(mbiRSZCHK) == (mbiHUGE_U)(size_t)(mbiRSZCHK)));
#undef mbiRSZCHK
#endif
 /* ssize_t is POSIX (or mapped from Win32 SSIZE_T by us) */
#ifdef SSIZE_MAX
 mbCTA(basic_ssizet_mask, mbiMASK_CHK(SSIZE_MAX));
 mbCTA(basic_ssizet,
	sizeof(ssize_t) == sizeof(size_t) &&
	!mbiTYPE_ISF(ssize_t) && !mbiTYPE_ISU(ssize_t) &&
	mbiMASK_BITS(SSIZE_MAX) >= mbiMASK_BITS(INT_MAX) &&
	mbiMASK_BITS(SSIZE_MAX) <= mbiMASK_BITS(mbiHUGE_S_MAX) &&
	((mbiHUGE_S)(SSIZE_MAX) == (mbiHUGE_S)(ssize_t)(SSIZE_MAX)) &&
	mbiMASK_BITS(SSIZE_MAX) < mbiTYPE_UBITS(size_t));
#ifdef SSIZE_MIN
 mbCTA(basic_ssizet_min,
	(SSIZE_MIN) < 0 &&
	((SSIZE_MIN) == -(SSIZE_MAX) || (SSIZE_MIN)+1 == -(SSIZE_MAX)) &&
	((SSIZE_MIN) == -(SSIZE_MAX)) == ((SCHAR_MIN) == -(SCHAR_MAX)));
#endif
#ifdef SIZE_MAX
 mbCTA(basic_ssizet_sizet,
	mbiMASK_BITS(SSIZE_MAX) <= mbiMASK_BITS(SIZE_MAX));
#endif /* SSIZE_MAX && SIZE_MAX */
#endif /* SSIZE_MAX */
 /* uintptr_t is C99 */
#if defined(UINTPTR_MAX) && !defined(__CHERI__)
 mbCTA(basic_uintptr_mask, mbiMASK_CHK(UINTPTR_MAX));
 mbCTA(basic_uintptr,
	sizeof(uintptr_t) >= sizeof(ptrdiff_t) &&
	sizeof(uintptr_t) >= sizeof(size_t) &&
	sizeof(uintptr_t) <= sizeof(mbiHUGE_U) &&
	!mbiTYPE_ISF(uintptr_t) && mbiTYPE_ISU(uintptr_t) &&
	mbiMASK_BITS(UINTPTR_MAX) == mbiTYPE_UBITS(uintptr_t) &&
	((mbiHUGE_U)(UINTPTR_MAX) == (mbiHUGE_U)(uintptr_t)(UINTPTR_MAX)) &&
	mbiTYPE_UBITS(uintptr_t) >= mbiMASK_BITS(UINT_MAX) &&
	((mbiHUGE_U)(UINTPTR_MAX) >= (mbiHUGE_U)(mbiSIZE_MAX)) &&
	mbiTYPE_UBITS(uintptr_t) >= mbiTYPE_UBITS(size_t) &&
	mbiTYPE_UBITS(uintptr_t) <= mbiMASK_BITS(mbiHUGE_U_MAX));
#ifdef PTRDIFF_MAX
 mbCTA(basic_uintptr_pdt,
	mbiMASK_BITS(UINTPTR_MAX) >= mbiMASK_BITS(PTRDIFF_MAX));
#endif /* UINTPTR_MAX && PTRDIFF_MAX */
#ifdef SIZE_MAX
 mbCTA(basic_uintptr_sizet,
	mbiMASK_BITS(UINTPTR_MAX) >= mbiMASK_BITS(SIZE_MAX));
#endif /* UINTPTR_MAX && SIZE_MAX */
#endif /* UINTPTR_MAX */
 /* mbiPTR_U is defined by us */
 mbCTA(basic_mbiptr_mask, mbiMASK_CHK(mbiPTR_U_MAX));
 mbCTA(basic_mbiptr,
	sizeof(mbiPTR_U) >= sizeof(ptrdiff_t) &&
	sizeof(mbiPTR_U) >= sizeof(size_t) &&
	sizeof(mbiPTR_U) <= sizeof(mbiHUGE_U) &&
	!mbiTYPE_ISF(mbiPTR_U) && mbiTYPE_ISU(mbiPTR_U) &&
	mbiMASK_BITS(mbiPTR_U_MAX) == mbiTYPE_UBITS(mbiPTR_U) &&
	((mbiHUGE_U)(mbiPTR_U_MAX) == (mbiHUGE_U)(mbiPTR_U)(mbiPTR_U_MAX)) &&
	mbiTYPE_UBITS(mbiPTR_U) >= mbiMASK_BITS(UINT_MAX) &&
	((mbiHUGE_U)(mbiPTR_U_MAX) >= (mbiHUGE_U)(mbiSIZE_MAX)) &&
	mbiTYPE_UBITS(mbiPTR_U) >= mbiTYPE_UBITS(size_t) &&
	mbiTYPE_UBITS(mbiPTR_U) <= mbiMASK_BITS(mbiHUGE_U_MAX));
#ifdef PTRDIFF_MAX
 mbCTA(basic_mbiptr_pdt,
	mbiMASK_BITS(mbiPTR_U_MAX) >= mbiMASK_BITS(PTRDIFF_MAX));
#endif
#ifdef SIZE_MAX
 mbCTA(basic_mbiptr_sizet,
	mbiMASK_BITS(mbiPTR_U_MAX) >= mbiMASK_BITS(SIZE_MAX));
#endif
 /* C99 §6.2.6.2(1, 2, 6) permits M ≤ N, but M < N is normally desired */
 /* here require signed/unsigned types to have same width (M=N-1) */
 mbCTA(vbits_char, mbiMASK_BITS(UCHAR_MAX) == mbiMASK_BITS(SCHAR_MAX) + 1U);
 mbCTA(vbits_short, mbiMASK_BITS(USHRT_MAX) == mbiMASK_BITS(SHRT_MAX) + 1U);
 mbCTA(vbits_int, mbiMASK_BITS(UINT_MAX) == mbiMASK_BITS(INT_MAX) + 1U);
 mbCTA(vbits_long, mbiMASK_BITS(ULONG_MAX) == mbiMASK_BITS(LONG_MAX) + 1U);
#ifdef UQUAD_MAX
 mbCTA(vbits_quad, mbiMASK_BITS(UQUAD_MAX) == mbiMASK_BITS(QUAD_MAX) + 1U);
#endif
#ifdef _UI64_MAX
 mbCTA(vbits_i64, mbiMASK_BITS(_UI64_MAX) == mbiMASK_BITS(_I64_MAX) + 1U);
#endif
#ifdef LLONG_MIN
 mbCTA(vbits_llong, mbiMASK_BITS(ULLONG_MAX) == mbiMASK_BITS(LLONG_MAX) + 1U);
#endif
#ifdef INTMAX_MIN
 mbCTA(vbits_imax, mbiMASK_BITS(UINTMAX_MAX) == mbiMASK_BITS(INTMAX_MAX) + 1U);
#endif
 mbCTA(vbits_large, mbiMASK_BITS(mbiLARGE_U_MAX) == mbiMASK_BITS(mbiLARGE_S_MAX) + 1U);
 mbCTA(vbits_huge, mbiMASK_BITS(mbiHUGE_U_MAX) == mbiMASK_BITS(mbiHUGE_S_MAX) + 1U);
#ifdef SSIZE_MAX
 mbCTA(vbits_size, mbiTYPE_UBITS(size_t) == mbiMASK_BITS(SSIZE_MAX) + 1U);
#endif
#if !defined(__CHERI__) && defined(UINTPTR_MAX) && defined(PTRDIFF_MAX)
 mbCTA(vbits_iptr, mbiMASK_BITS(UINTPTR_MAX) == mbiMASK_BITS(PTRDIFF_MAX) + 1U);
#endif
 /* do size_t and ulong fit each other? */
 mbCTA(sizet_minint, sizeof(size_t) >= sizeof(int)); /* for 16-bit systems */
#if MBSDINT_H_WANT_LONG_IN_SIZET
 mbCTA(sizet_minlong, sizeof(size_t) >= sizeof(long));
#endif
#ifdef MBSDINT_H_WANT_SIZET_IN_LONG
 /* with MBSDINT_H_WANT_PTR_IN_SIZET breaks LLP64 (e.g. Windows/amd64) */
 mbCTA(sizet_inulong, sizeof(size_t) <= sizeof(long));
#endif
 /* this is as documented above */
 mbCTA(sizet_inlarge, sizeof(size_t) <= sizeof(mbiLARGE_U));
#if MBSDINT_H_MBIPTR_IN_LARGE
 mbCTA(mbiPTRU_inlarge,
	sizeof(mbiPTR_U) <= sizeof(mbiLARGE_U) &&
	mbiTYPE_UBITS(mbiPTR_U) <= mbiMASK_BITS(mbiLARGE_U_MAX));
#endif
 /* assume ptrdiff_t and (s)size_t will fit each other */
 mbCTA(sizet_ptrdiff, sizeof(size_t) == sizeof(ptrdiff_t));
#ifdef SSIZE_MAX
 mbCTA(sizet_ssize, sizeof(size_t) == sizeof(ssize_t));
#endif
 /* also mbiPTR_U, to rule out certain weird machines */
#if MBSDINT_H_MBIPTR_IS_SIZET
 /* note mbiMASK_BITS(SIZE_MAX) could be smaller still */
 mbCTA(sizet_mbiPTRU,
	sizeof(mbiPTR_U) == sizeof(size_t) &&
	mbiTYPE_UBITS(mbiPTR_U) == mbiTYPE_UBITS(size_t));
 /* more loose bonus check */
#if defined(UINTPTR_MAX) && !defined(__CHERI__)
 mbCTA(sizet_uintptr, sizeof(size_t) == sizeof(uintptr_t));
#endif
#endif
 /* user-requested extra checks */
#ifdef MBSDINT_H_WANT_INT32
 /* guaranteed by POSIX */
 mbCTA(user_int32, mbiTYPE_UBITS(unsigned int) >= 32);
#endif
#ifdef MBSDINT_H_WANT_LRG64
 /* guaranteed if 64-bit types even exist */
 mbCTA(user_lrg64, mbiTYPE_UBITS(mbiLARGE_U) >= 64);
#endif
#ifdef MBSDINT_H_WANT_SAFEC
 /* true as of C23 and POSIX unless INT_MIN == -INT_MAX */
 mbCTA(user_safec, mbiSAFECOMPLEMENT == 1);
#endif
#ifdef MBSDINT_H_WANT_PTR_IN_SIZET
 /* require pointers and size_t to take up the same amount of space */
 mbCTA(sizet_voidptr, sizeof(size_t) == sizeof(void *));
 mbCTA(sizet_sintptr, sizeof(size_t) == sizeof(int *));
 mbCTA(sizet_funcptr, sizeof(size_t) == sizeof(void (*)(void)));
#endif
 /* C23 §5.2.4.2.1 */
#ifdef CHAR_WIDTH
 mbCTA(char_width, (CHAR_WIDTH) == (CHAR_BIT));
#endif
#ifdef SCHAR_WIDTH
 mbCTA(schar_width, (SCHAR_WIDTH) == (CHAR_BIT));
#endif
#ifdef UCHAR_WIDTH
 mbCTA(uchar_width, (UCHAR_WIDTH) == (CHAR_BIT));
#endif
#ifdef USHRT_WIDTH
 mbCTA(ushrt_width, (USHRT_WIDTH) == mbiMASK_BITS(USHRT_MAX));
#endif
#ifdef SHRT_WIDTH
 mbCTA(shrt_width, (SHRT_WIDTH) == mbiMASK_BITS(SHRT_MAX) + 1);
#endif
#ifdef UINT_WIDTH
 mbCTA(uint_width, (UINT_WIDTH) == mbiMASK_BITS(UINT_MAX));
#endif
#ifdef INT_WIDTH
 mbCTA(int_width, (INT_WIDTH) == mbiMASK_BITS(INT_MAX) + 1);
#endif
#ifdef ULONG_WIDTH
 mbCTA(ulong_width, (ULONG_WIDTH) == mbiMASK_BITS(ULONG_MAX));
#endif
#ifdef LONG_WIDTH
 mbCTA(long_width, (LONG_WIDTH) == mbiMASK_BITS(LONG_MAX) + 1);
#endif
#ifdef ULLONG_WIDTH
 mbCTA(ullong_width, (ULLONG_WIDTH) == mbiMASK_BITS(ULLONG_MAX));
#endif
#ifdef LLONG_WIDTH
 mbCTA(llong_width, (LLONG_WIDTH) == mbiMASK_BITS(LLONG_MAX) + 1);
#endif
#ifdef UINTMAX_WIDTH
 mbCTA(uintmax_width, (UINTMAX_WIDTH) == mbiMASK_BITS(UINTMAX_MAX));
#endif
#ifdef INTMAX_WIDTH
 mbCTA(intmax_width, (INTMAX_WIDTH) == mbiMASK_BITS(INTMAX_MAX) + 1);
#endif
#if defined(UINTPTR_WIDTH) && !defined(__CHERI__)
 mbCTA(uintptr_width, (UINTPTR_WIDTH) == mbiMASK_BITS(UINTPTR_MAX));
#endif
#ifdef PTRDIFF_WIDTH
 mbCTA(ptrdiff_width, (PTRDIFF_WIDTH) == mbiMASK_BITS(PTRDIFF_MAX) + 1);
#endif
#ifdef SIZE_WIDTH
 mbCTA(size_width, (SIZE_WIDTH) == mbiMASK_BITS(SIZE_MAX));
#endif
 /* require nōn-cpp (possibly casted) match if extant */
#ifdef mbiHUGE_UBITS
 mbCTA(huge_width, mbiHUGE_UBITS == mbiMASK_BITS(mbiHUGE_U_MAX));
#endif
#ifdef mbiLARGE_UBITS
 mbCTA(large_width, mbiLARGE_UBITS == mbiMASK_BITS(mbiLARGE_U_MAX));
#endif
mbCTA_END(mbsdint_h);
#endif /* !MBSDINT_H_SKIP_CTAS */

/* cpp-safe */
#ifndef mbiHUGE_UBITS
#define mbiHUGE_UBITS		mbiMASK__BITS(mbiHUGE_U_MAX)
#endif
#ifndef mbiLARGE_UBITS
#define mbiLARGE_UBITS		mbiMASK__BITS(mbiLARGE_U_MAX)
#endif

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
 * masks: HM = half mask (e.g. FM >> 1 or 0x7FFFFFFFUL) == signed _MAX
 *	  FM = full mask (e.g. ULONG_MAX or 0xFFFFFFFFUL)
 *	  tM = type-corresponding mask
 *
 * Where unsigned values may be either properly unsigned or
 * manually represented in two’s complement (M=N-1 above),
 * signed values are the host’s (and conversion will UB for
 * -type_MAX-1 unless the mbiSAFECOMPLEMENT macro equals 1,
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

/* 4. helpers */
#define mbiOUneg(ut,v)		mbmscWd(4146) \
				mbiOU1(ut, -, (v)) \
				mbmscWpop

/* manual two’s complement in unsigned arithmetics */

/* 1. obtain sign of encoded value */
#define mbiA_S2VZ(v)		((v) < 0)
#define mbiA_U2VZ(ut,HM,v)	mbiCOU(ut, (v), >, (HM))
/* masking */
#define mbiMA_U2VZ(ut,FM,HM,v)	mbiCOU(ut, mbiMM(ut, (FM), (v)), >, (HM))

/* 2. casting between unsigned(two’s complement) and signed(native) */
#define mbiA_U2S(ut,st,HM,v)						\
		mbiOT(st,						\
		 mbiA_U2VZ(ut, (HM), (v)),				\
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

#define mbiA_U2M(ut,HM,v)						\
		mbiOT(ut,						\
		 mbiA_U2VZ(ut, (HM), (v)),				\
		 mbiOUneg(ut, (v)),					\
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
#define mbiA_VZM2U(ut,HM,vz,m)						\
		mbiOT(ut,						\
		 (vz) && ((m) > 0U),					\
		 mbiOU1(ut,						\
		  ~,							\
		  mbiMO(ut, (HM), (m), -, 1U)				\
		 ),							\
		 mbiMM(ut, (HM), (m))					\
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
		 mbiOUneg(ut, (m)),					\
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
#define mbi__totypeof(templval,dstval)	((1 ? 0 : (templval)) + (dstval))
#define mbi__halftmax(stmax)		mbi__totypeof(stmax, \
	(mbi__totypeof(stmax, 1) << (mbiMASK_BITS(stmax) / 2U)))
/* note: must constant-evaluate even for signed ut */
#define mbi__halftype(ut)		mbiOT(ut, mbiTYPE_ISU(ut), \
	mbiOshl(ut, 1U, mbiTYPE_UBITS(ut) / 2U), 2)
#define mbi__uabovehalftype(ut,a,b)	(((ut)a | (ut)b) >= mbi__halftype(ut))
/* assert: !(b < 0) */
#define mbi__sabovehalftype(stmax,a,b)	(a < 0 ? 1 : \
	(mbi__totypeof(stmax, a) | mbi__totypeof(stmax, b)) >= mbi__halftmax(stmax))

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
	if (__predict_false((vr) != 0 &&				\
	    mbi__uabovehalftype(ut, (vl), (vr)) &&			\
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
	if (__predict_false((vr) != 0 &&				\
	    mbi__sabovehalftype(lim ## _MAX, (vl), (vr)) && ((vl) < 0 ?	\
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
	if (((vr) >= 0) ? __predict_false((vr) != 0 &&			\
	    mbi__sabovehalftype(lim ## _MAX, (vl), (vr)) && ((vl) < 0 ?	\
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

/* 4. safe to use mbiA_U2S / mbiA_VZM2S for the given in-range value */
#if mbiSAFECOMPLEMENT
#define mbiCAsafeU2S(lim,ut,v) do {					\
	/* nothing */ (void)(ut)(v); (void)(lim ## _MAX);		\
} while (/* CONSTCOND */ 0)
#define mbiCAsafeVZM2S(lim,ut,vz,m) do {				\
	/* nothing */ (void)(ut)(m); (void)(lim ## _MAX); (void)(vz);	\
} while (/* CONSTCOND */ 0)
#else /* one’s complement or sign-and-magnitude or -type_MAX-1 is a trap */
#define mbiCAsafeU2S(lim,ut,v) do {					\
	if (__predict_false((ut)(v) == mbiOU(ut, lim ## _MAX, +, 1)))	\
		mbiCfail;						\
} while (/* CONSTCOND */ 0)
#define mbiCAsafeVZM2S(lim,ut,vz,m) do {				\
	if ((vz) &&							\
	    __predict_false((ut)(m) == mbiOU(ut, lim ## _MAX, +, 1)))	\
		mbiCfail;						\
} while (/* CONSTCOND */ 0)
#endif

/*
 * calculations on manual two’s k̲omplement signed numbers in unsigned vars
 *
 * addition, subtraction and multiplication, bitwise and boolean
 * operations, as well as equality comparisons can be done on the
 * unsigned value; other operations follow:
 */

/* k-signed compare < <= > >= */
#define mbiK_signbit(ut,HM)	mbiOU(ut, (HM), +, 1)
#define mbiK_signflip(ut,HM,v)	mbiOU(ut, (v), ^, mbiK_signbit(ut, (HM)))
#define mbiKcmp(ut,HM,vl,op,vr)	mbiCOU(ut, \
		mbiK_signflip(ut, (HM), (vl)), \
		op, \
		mbiK_signflip(ut, (HM), (vr)))
#define mbiMKcmp(ut,FM,HM,vl,op,vr) \
	mbiKcmp(ut, (HM), mbiMM(ut, (FM), (vl)), op, mbiMM(ut, (FM), (vr)))

/* rotate and shift (can also be used on unsigned values) */
#define mbiKrol(ut,vl,vr)	mbiK_sr(ut, mbiK_rol, (vl), (vr), void)
#define mbiKror(ut,vl,vr)	mbiK_sr(ut, mbiK_ror, (vl), (vr), void)
#define mbiKshl(ut,vl,vr)	mbiK_sr(ut, mbiK_shl, (vl), (vr), void)
/* let vz be sgn(vl): mbi{,M}A_U2VZ(ut,HM,vl) */
/* mbiA_U2VZ(ut,mbiTYPE_UMAX(ut)>>1,vl) if unsigned and nōn-masking */
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

/* k-signed division and remainder */
#define mbiKdiv(ut,HM,vl,vr)		mbiA_VZU2U(ut, \
	mbiA_U2VZ(ut, (HM), (vl)) ^ mbiA_U2VZ(ut, (HM), (vr)), \
	mbiUI(mbiA_U2M(ut, (HM), (vl))) / mbiUI(mbiA_U2M(ut, (HM), (vr))))
#define mbiMK_div(ut,FM,HM,vl,vr)	mbiA_VZU2U(ut, \
	mbiMA_U2VZ(ut, (FM), (HM), (vl)) ^ mbiMA_U2VZ(ut, (FM), (HM), (vr)), \
	mbiUI(mbiMA_U2M(ut, (FM), (HM), (vl))) / mbiUI(mbiMA_U2M(ut, (FM), (HM), (vr))))
#define mbiMKdiv(ut,FM,HM,vl,vr)	mbiMM(ut, (FM), \
	mbiMK_div(ut, (FM), (HM), (vl), (vr)))
#define mbiK_rem(ut,vl,vr,vdiv)		mbiOU(ut, vl, -, mbiOU(ut, vdiv, *, vr))
#define mbiKrem(ut,HM,vl,vr)		mbiK_rem(ut, (vl), (vr), \
					    mbiKdiv(ut, (HM), (vl), (vr)))
#define mbiMKrem(ut,FM,HM,vl,vr)	mbiMM(ut, (FM), mbiK_rem(ut, (vl), (vr), \
					    mbiMK_div(ut, (FM), (HM), (vl), (vr))))
/* statement, assigning to dstdiv and dstrem */
#define mbiKdivrem(dstdiv,dstrem,ut,HM,vl,vr) do {			\
	ut mbi__TMP = mbiKdiv(ut, (HM), (vl), (vr));			\
	(dstdiv) = mbi__TMP;						\
	(dstrem) = mbiK_rem(ut, (vl), (vr), mbi__TMP);			\
} while (/* CONSTCOND */ 0)
#define mbiMKdivrem(dstdiv,dstrem,ut,FM,HM,vl,vr) do {			\
	ut mbi__TMP = mbiMK_div(ut, (FM), (HM), (vl), (vr));		\
	(dstdiv) = mbiMM(ut, (FM), mbi__TMP);				\
	(dstrem) = mbiMM(ut, (FM), mbiK_rem(ut, (vl), (vr), mbi__TMP));	\
} while (/* CONSTCOND */ 0)

/*
 * anything else
 */

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif /* !SYSKERN_MBSDINT_H */
