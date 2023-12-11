/*-
 * MirBSD ISO C compiler toolkit (macros for mbsdint.h and others)
 *
 * © mirabilos Ⓕ CC0 or The MirOS Licence (MirBSD)
 */

#ifndef SYSKERN_MBSDCC_H
#define SYSKERN_MBSDCC_H "$MirOS: src/bin/mksh/mbsdcc.h,v 1.8 2023/12/11 11:57:40 tg Exp $"

/*
 * Note: this header uses the SIZE_MAX (or similar) definitions
 * from <basetsd.h> or <limits.h> if present, so either include
 * these before this header or include "mbsdint.h", which fixes
 * up the corresponding definition.
 *
 * Furthermore, either include <stdlib.h> for abort(3), or pro‐
 * vide an mbccABEND macro (preferred).
 */

#if !defined(_KERNEL) && !defined(_STANDALONE)
#include <stddef.h>
#else
#include <sys/stddef.h>
#endif

/* mbccABEND should be provided by the user, called by mbsd*.h only */
#ifndef mbccABEND
#define mbccABEND(reasonstr) abort()
#endif

/* monkey-patch known-bad offsetof versions */
#if (defined(__KLIBC__) || defined(__dietlibc__)) && \
    ((defined(__GNUC__) && (__GNUC__ > 3)) || defined(__NWCC__))
#undef offsetof
#define offsetof(struc,memb)	__builtin_offsetof(struc, memb)
#endif

/* mbsdint.h replaces this with mbiSIZE_MAX */
#undef mbccSIZE_MAX
#if defined(SIZE_MAX)
#define mbccSIZE_MAX		((size_t)(SIZE_MAX)
#elif defined(SIZE_T_MAX)
#define mbccSIZE_MAX		((size_t)(SIZE_T_MAX)
#elif defined(MAXSIZE_T)
#define mbccSIZE_MAX		((size_t)(MAXSIZE_T)
#else
#define mbccSIZE_MAX		((size_t)~(size_t)0)
#endif

#ifdef __cplusplus
#define mbcextern		extern "C"
/* for headers (BSD __BEGIN_DECLS/__END_DECLS) */
#define mbcextern_beg		extern "C" {
#define mbcextern_end		}
#else
#define mbcextern		extern
#define mbcextern_beg
#define mbcextern_end
#endif

/* compiler warning pragmas, NOP for other compilers */
#ifdef _MSC_VER
/* push, suppress space-separated listed warning numbers */
#define mbmscWd(d)		__pragma(warning(push)) \
				__pragma(warning(disable: d))
/* suppress for only the next line */
#define mbmscWs(d)		__pragma(warning(suppress: d))
/* pop from mbmscWd */
#define mbmscWpop		__pragma(warning(pop))
#else
#define mbmscWd(d)
#define mbmscWs(d)
#define mbmscWpop
#endif

/* in-expression compile-time check evaluating to 0 */
#ifdef __cplusplus
template<bool> struct mbccChkExpr_sa;
template<> struct mbccChkExpr_sa<true>{};
#define mbccChkExpr(test)	(sizeof((mbccChkExpr_sa<!!(0+(test))>())) * 0)
#else
#define mbccChkExpr(test)	mbmscWs(4116) \
				(sizeof(struct { int (mbccChkExpr):((0+(test)) ? 1 : -1); }) * 0)
#endif

/* ensure value x is a constant expression */
#ifdef __cplusplus
template<bool,bool> struct mbccCEX_sa;
template<> struct mbccCEX_sa<true,false>{};
template<> struct mbccCEX_sa<false,true>{};
#define mbccCEX(x)		(sizeof((mbccCEX_sa<!!(0+(x)), !(0+(x))>())) * 0 + (x))
#else
#define mbccCEX(x)		mbmscWs(4116) \
				(sizeof(struct { int (mbccCEX):(((0+(x)) && 1) + 1); }) * 0 + (x))
#endif

/* flexible array member */
#if defined(__cplusplus)
#ifdef __cpp_flexible_array_members
#define mbccFAMslot(type,memb)	type memb[]
#else
#define mbccFAMslot(type,memb)	type memb[0] mbmscWs(4200 4820)
#endif
#elif (defined(__STDC_VERSION__) && ((__STDC_VERSION__) >= 199901L))
#define mbccFAMslot(type,memb)	type memb[]
#elif defined(__GNUC__)
#define mbccFAMslot(type,memb)	type memb[0] mbmscWs(4200 4820)
#else
#define mbccFAMslot(type,memb)	type memb[1] mbmscWs(4820) /* SOL */
#endif
#define mbccFAMsz_i(t,memb,sz)	/*private*/ (size_t)(offsetof(t, memb) + (size_t)sz)
/* compile-time constant */
#define mbccFAMSZ(struc,memb,sz) ((size_t)( \
	    mbccChkExpr(mbccSIZE_MAX - sizeof(struc) > mbccCEX((size_t)(sz))) + \
	    (mbccFAMsz_i(struc, memb, (sz)) > sizeof(struc) ? \
	     mbccFAMsz_i(struc, memb, (sz)) : sizeof(struc))))
/* run-time */
#define mbccFAMsz(struc,memb,sz) ((size_t)( \
	    !(mbccSIZE_MAX - sizeof(struc) > (size_t)(sz)) ? \
	    (mbccABEND("mbccFAMsz: " mbccS(sz) " too large for " mbccS(struc) "." mbccS(memb)), 0) : \
	    (mbccFAMsz_i(struc, memb, (sz)) > sizeof(struc) ? \
	     mbccFAMsz_i(struc, memb, (sz)) : sizeof(struc))))
/* example:
 *
 * struct s {
 *	int type;
 *	mbccFAMslot(char, label);	// like char label[…];
 * };
 * struct t {
 *	int count;
 *	mbccFAMslot(time_t, value);
 * };
 * union fixed_s {
 *	struct s s;
 *	char storage[mbccFAMSZ(struct s, label, 28)];
 * };
 * struct s *sp = malloc(mbccFAMsz(struct s, label, strlen(labelvar) + 1U));
 * struct t *tp = malloc(mbccFAMsz(struct t, value, sizeof(time_t[cnt])));
 * tp->count = cnt;
 * struct s *np = malloc(mbccFAMSZ(struct s, label, sizeof("myname")));
 * memcpy(np->label, "myname", sizeof("myname"));
 * static union fixed_s preallocated;
 * preallocated.s.label[0] = '\0';
 * somefunc((struct s *)&preallocated);
 */

/* field sizeof */
#if defined(__cplusplus)
/* TODO: add better way on CFrustFrust once it has one */
#define mbccFSZ(struc,memb)	(sizeof(((struc *)0)->memb))
#elif (defined(__STDC_VERSION__) && ((__STDC_VERSION__) >= 199901L))
#define mbccFSZ(struc,memb)	(sizeof(((struc){0}).memb))
#else
#define mbccFSZ(struc,memb)	(sizeof(((struc *)0)->memb))
#endif

/* stringification with expansion */
#define mbccS(x)		#x
#define mbccS2(x)		mbccS(x)

/* compile-time assertions */
#undef mbccCTA
#if defined(__cplusplus)
#ifdef __cpp_static_assert
#define mbccCTA(fldn,cond)	static_assert(cond, mbccS(fldn))
#endif
#elif (defined(__STDC_VERSION__) && ((__STDC_VERSION__) >= 202311L))
#define mbccCTA(fldn,cond)	static_assert(cond, mbccS(fldn))
#elif (defined(__STDC_VERSION__) && ((__STDC_VERSION__) >= 201112L))
#define mbccCTA(fldn,cond)	_Static_assert(cond, mbccS(fldn))
#elif defined(__GNUC__) && \
    ((__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
#define mbccCTA(fldn,cond)	__extension__ _Static_assert(cond, mbccS(fldn))
#endif
#ifndef mbccCTA
/* single assertion for macros (needs fldn prefixed, cond parenthesised) */
#define mbccCTA(fldn,cond)	unsigned char fldn:(cond ? 1 : -1)
/* begin/end assertion block */
#define mbCTA_BEG(name)		struct ctassert_ ## name {		\
					char ctabeg /* plus user semicolon */
#define mbCTA_END(name)			char ctaend;			\
				};					\
				struct ctassert2 ## name {		\
					char ok[sizeof(struct ctassert_ ## name) > 2 ? 1 : -1]; \
				} /* semicolon provided by user */
#else
/* nothing, but syntax-check equally to manual compile-time assert macro */
#define mbCTA_BEG(name)		struct ctassert_ ## name { char ctabeg; }
#define mbCTA_END(name)		struct ctassert2 ## name { char ctaend; }
#endif
/* single assertion */
#define mbCTA(name,cond)	mbccCTA(cta_ ## name, (cond))

/* nil pointer constant */
#if (defined(__cplusplus) && (__cplusplus >= 201103L)) || \
    (defined(__STDC_VERSION__) && ((__STDC_VERSION__) >= 202311L))
#define mbnil			nullptr
#elif defined(__GNUG__)
#define mbnil			__null
#elif defined(__cplusplus)
#define mbnil			(size_t)0UL	/* wtf, why? */
#else
#define mbnil			(void *)0UL
#endif

#endif /* !SYSKERN_MBSDCC_H */
