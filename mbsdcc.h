/*-
 * MirBSD ISO C compiler toolkit (macros for mbsdint.h and others)
 *
 * © mirabilos Ⓕ CC0 or The MirOS Licence (MirBSD)
 */

#ifndef SYSKERN_MBSDCC_H
#define SYSKERN_MBSDCC_H "$MirOS: src/bin/mksh/mbsdcc.h,v 1.2 2023/09/17 00:47:51 tg Exp $"

#if !defined(_KERNEL) && !defined(_STANDALONE)
#include <stddef.h>
#else
#include <sys/stddef.h>
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

/* flexible array member (use with offsetof, NOT C99-style sizeof!) */
#if (defined(__STDC_VERSION__) && ((__STDC_VERSION__) >= 199901L)) || \
    defined(__cpp_flexible_array_members)
#define mbccFAM(type,name)	type name[]
#elif defined(__GNUC__) || defined(__cplusplus)
#define mbccFAM(type,name)	type name[0] mbmscWs(4200 4820)
#else
#define mbccFAM(type,name)	type name[1] mbmscWs(4820) /* SOL */
#endif
/* example:
 *
 * struct s {
 *	int size;
 *	mbccFAM(char, label);	// like char label[…];
 * };
 * struct s *sp = malloc(offsetof(struct s, label[0]) + labellen);
 */

/* field sizeof */
#if (defined(__STDC_VERSION__) && ((__STDC_VERSION__) >= 199901L))
#define mbccFSZ(struc,memb)	(sizeof(((struc){0}).memb))
#else
/* TODO: add better way on CFrustFrust once it has one */
#define mbccFSZ(struc,memb)	(sizeof(((struc *)0)->memb))
#endif

/* stringification with expansion */
#define mbccS(x)		#x

/* compile-time assertions */
#undef mbccCTA
#if (defined(__STDC_VERSION__) && ((__STDC_VERSION__) >= 202311L)) || \
    defined(__cpp_static_assert)
#define mbccCTA(fldn,cond)	static_assert(cond, mbccS(fldn))
#elif (defined(__STDC_VERSION__) && ((__STDC_VERSION__) >= 201112L))
#define mbccCTA(fldn,cond)	_Static_assert(cond, mbccS(fldn))
#elif !defined(__cplusplus) && defined(__GNUC__) && ((__GNUC__ > 4) || \
    (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
#define mbccCTA(fldn,cond)	__extension__ _Static_assert(cond, mbccS(fldn))
#endif
#ifndef mbccCTA
/* single assertion for macros (prefix fldn suitably and parenthesise cond) */
#define mbccCTA(fldn,cond)	unsigned int fldn:(cond ? 1 : -1)
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
