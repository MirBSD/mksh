/*	$OpenBSD: c_ulimit.c,v 1.19 2013/11/28 10:33:37 sobrado Exp $	*/

/*-
 * Copyright (c) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009,
 *		 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017,
 *		 2019, 2020, 2021, 2022, 2023
 *	mirabilos <m@mirbsd.org>
 *
 * Provided that these terms and disclaimer and all copyright notices
 * are retained or reproduced in an accompanying document, permission
 * is granted to deal in this work without restriction, including un-
 * limited rights to use, publicly perform, distribute, sell, modify,
 * merge, give away, or sublicence.
 *
 * This work is provided "AS IS" and WITHOUT WARRANTY of any kind, to
 * the utmost extent permitted by applicable law, neither express nor
 * implied; without malicious intent or gross negligence. In no event
 * may a licensor, author or contributor be held liable for indirect,
 * direct, other damage, loss, or other issues arising in any way out
 * of dealing in the work, even if advised of the possibility of such
 * damage or existence of a defect, except proven that it results out
 * of said person's immediate fault when using the work as intended.
 */

#include "sh.h"

__RCSID("$MirOS: src/bin/mksh/ulimit.c,v 1.17 2023/12/13 14:52:18 tg Exp $");

#define SOFT	0x1
#define HARD	0x2

#if HAVE_RLIMIT

/* Magic to divine the 'm' and 'v' limits */

#undef MKHL_AS
#undef MKHL_VMEM
#undef MKHL_RSS

#ifdef RLIMIT_AS
#define MKHL_AS
#endif

#ifdef RLIMIT_VMEM
#define MKHL_VMEM
#endif

#ifdef RLIMIT_RSS
#define MKHL_RSS
#endif

#if defined(MKHL_AS) && defined(MKHL_VMEM) && (RLIMIT_AS == RLIMIT_VMEM)
#undef MKHL_VMEM
#endif

#if defined(MKHL_AS) && defined(MKHL_RSS) && (RLIMIT_AS == RLIMIT_RSS)
#undef MKHL_RSS
#endif

#if defined(MKHL_VMEM) && defined(MKHL_RSS) && (RLIMIT_VMEM == RLIMIT_RSS)
#undef MKHL_VMEM
#endif

#undef ULIMIT_V_IS_AS
#undef ULIMIT_V_IS_VMEM
#undef ULIMIT_M_IS_RSS
#undef ULIMIT_M_IS_VMEM

#ifdef MKHL_AS
#define ULIMIT_V_IS_AS
#if defined(MKHL_RSS)
#define ULIMIT_M_IS_RSS
#elif defined(MKHL_VMEM)
#define ULIMIT_M_IS_VMEM
#endif
#else /* !MKHL_AS */
#ifdef MKHL_VMEM
#define ULIMIT_V_IS_VMEM
#endif
#ifdef MKHL_RSS
#define ULIMIT_M_IS_RSS
#endif
#endif /* !MKHL_AS */

/* pkgsrc® at least on Mac OSX expects -m as -v alias */
#if !defined(ULIMIT_M_IS_RSS) && !defined(ULIMIT_M_IS_VMEM)
#if defined(RLIMIT_VMEM)
#define ULIMIT_M_IS_VMEM
#elif defined(RLIMIT_RSS)
#define ULIMIT_M_IS_RSS
#endif
#endif

#undef MKHL_AS
#undef MKHL_VMEM
#undef MKHL_RSS

#define LIMITS_GEN	"rlimits.gen"

#else /* !HAVE_RLIMIT */

#undef RLIMIT_CORE	/* just in case */

#if defined(UL_GETFSIZE)
#define KSH_UL_GFIL	UL_GETFSIZE
#elif defined(UL_GFILLIM)
#define KSH_UL_GFIL	UL_GFILLIM
#elif defined(__A_UX__) || defined(KSH_ULIMIT2_TEST)
#define KSH_UL_GFIL	1
#endif

#if defined(UL_SETFSIZE)
#define KSH_UL_SFIL	UL_SETFSIZE
#elif defined(UL_SFILLIM)
#define KSH_UL_SFIL	UL_SFILLIM
#elif defined(__A_UX__) || defined(KSH_ULIMIT2_TEST)
#define KSH_UL_SFIL	2
#endif

#if defined(KSH_UL_SFIL)
#define KSH_UL_WFIL	Ja
#else
#define KSH_UL_WFIL	Nee
#define KSH_UL_SFIL	0
#endif

#if defined(UL_GETMAXBRK)
#define KSH_UL_GBRK	UL_GETMAXBRK
#elif defined(UL_GMEMLIM)
#define KSH_UL_GBRK	UL_GMEMLIM
#elif defined(__A_UX__) || defined(KSH_ULIMIT2_TEST)
#define KSH_UL_GBRK	3
#endif

#if defined(UL_GDESLIM)
#define KSH_UL_GDES	UL_GDESLIM
#elif defined(__GLIBC__) || defined(KSH_ULIMIT2_TEST)
#define KSH_UL_GDES	4
#endif

extern char etext;
extern long ulimit(int, long);

#define LIMITS_GEN	"ulimits.gen"

#endif /* !HAVE_RLIMIT */

struct limits {
	/* limit resource / read command */
	int resource;
#if HAVE_RLIMIT
	/* multiply by to get rlim_{cur,max} values */
	unsigned int factor;
#else
	/* write command */
	int wesource;
	/* writable? */
	Wahr writable;
#endif
	/* getopts char */
	char optchar;
	/* limit name */
	char name[1];
};

#define RLIMITS_DEFNS
#if HAVE_RLIMIT
#define FN(lname,lid,lfac,lopt)				\
	static const struct {				\
		int resource;				\
		unsigned int factor;			\
		char optchar;				\
		char name[sizeof(lname)];		\
	} rlimits_ ## lid = {				\
		lid, lfac, lopt, lname			\
	};
#else
#define FN(lname,lg,ls,lw,lopt)				\
	static const struct {				\
		int rcmd;				\
		int wcmd;				\
		Wahr writable;				\
		char optchar;				\
		char name[sizeof(lname)];		\
	} rlimits_ ## lg = {				\
		lg, ls, lw, lopt, lname			\
	};
#endif
#include LIMITS_GEN

static void print_ulimit(const struct limits *, int);
static int set_ulimit(const struct limits *, const char *, int);

/*
 * UGH! Strictly speaking this is UB in C. Need to figure out whether
 * it is worth the botherance to fix this (union) or to test for C99+
 * flexible array member using it when present, maybe keeping this if
 * not… :~
 */
static const struct limits * const rlimits[] = {
#define RLIMITS_ITEMS
#include LIMITS_GEN
};

static const char rlimits_opts[] =
#define RLIMITS_OPTCS
#include LIMITS_GEN
#ifndef RLIMIT_CORE
	"c"
#endif
    ;

int
c_ulimit(const char **wp)
{
	size_t i = 0;
	int how = SOFT | HARD, optc;
	char what = 'f';
	Wahr all = Nee;

	while ((optc = ksh_getopt(wp, &builtin_opt, rlimits_opts)) != -1)
		switch (optc) {
		case ORD('H'):
			how = HARD;
			break;
		case ORD('S'):
			how = SOFT;
			break;
		case ORD('a'):
			all = Ja;
			break;
		case ORD('?'):
 unknown_opt:
			kwarnf0(KWF_BIERR | KWF_NOERRNO,
			    "usage: ulimit [-%s] [value]", rlimits_opts);
			return (1);
		default:
			what = optc;
		}

	if (all) {
		if (wp[builtin_opt.optind]) {
 unexpected_args:
			kwarnf(KWF_BIERR | KWF_ONEMSG | KWF_NOERRNO,
			    Ttoo_many_args);
			return (1);
		}
		while (i < NELEM(rlimits)) {
			shprintf("-%c: %-20s  ", rlimits[i]->optchar,
			    rlimits[i]->name);
			print_ulimit(rlimits[i], how);
			++i;
		}
		return (0);
	}

	while (i < NELEM(rlimits)) {
		if (rlimits[i]->optchar == what)
			goto found;
		++i;
	}
#ifndef RLIMIT_CORE
	if (what == ORD('c'))
		/* silently accept */
		return (0);
#endif
	ksh_getopt_opterr(what, wp[0], Tunknown_option);
	goto unknown_opt;

 found:
	if (wp[builtin_opt.optind]) {
		if (wp[builtin_opt.optind + 1])
			goto unexpected_args;
		return (set_ulimit(rlimits[i], wp[builtin_opt.optind], how));
	}
	print_ulimit(rlimits[i], how);
	return (0);
}

#if HAVE_RLIMIT
#ifndef MKSH_RLIM_T
#define MKSH_RLIM_T rlim_t
#else
/* because add_cppflags does not handle spaces */
#define RLT_SI signed int
#define RLT_UI unsigned int
#define RLT_SL signed long
#define RLT_UL unsigned long
#define RLT_SQ signed long long
#define RLT_UQ unsigned long long
#endif
#define RL_T MKSH_RLIM_T
#define RL_U (MKSH_RLIM_T)RLIM_INFINITY
mbCTA_BEG(ulimit_c);
 mbCTA(rlinf_fits, (RLIM_INFINITY) == (RL_U));
mbCTA_END(ulimit_c);
#else
#define RL_T long
#define RL_U LONG_MAX
#endif

static int
set_ulimit(const struct limits *l, const char *v, int how MKSH_A_UNUSED)
{
	RL_T val = (RL_T)0;
	mbiHUGE_U hval;
#if HAVE_RLIMIT
	struct rlimit limit;
#endif

	if (strcmp(v, "unlimited") == 0) {
		val = RL_U;
		goto got_val;
	}
	if (!getnh(v, &hval)) {
		mksh_uari_t rval;

		if (errno != EINVAL)
			goto inv_val;

		if (!evaluate(v, (mksh_ari_t *)&rval, KSH_RETURN_ERROR, Nee))
			return (1);
		/*
		 * Avoid problems caused by typos that evaluate misses due
		 * to evaluating unset parameters to 0...
		 * If this causes problems, will have to add parameter to
		 * evaluate() to control if unset params are 0 or an error.
		 */
		if (!rval && !ctype(v[0], C_DIGIT)) {
			errno = EINVAL;
 inv_val:
			kwarnf0(KWF_BIERR, "invalid %s limit: %s", l->name, v);
			return (1);
		}
		hval = rval;
	}
	errno = EOVERFLOW;
#define mbiCfail goto inv_val
#if HAVE_RLIMIT
	mbiCAUmul(mbiHUGE_U, hval, l->factor);
#endif
	if (mbiTYPE_ISU(RL_T))
		mbiCASlet(RL_T, val, mbiHUGE_U, hval);
	else {
		/* huh, rlim_t is supposed to be unsigned! */
		mbiHUGE_S hsval;

		mbiCAsafeU2S(mbiHUGE_S, mbiHUGE_U, hval);
		hsval = mbiA_U2S(mbiHUGE_U, mbiHUGE_S, mbiHUGE_S_MAX, hval);
		mbiCASlet(RL_T, val, mbiHUGE_S, hsval);
	}
#undef mbiCfail
	/* do not numerically apprehend magic values */
	if (
#if HAVE_RLIMIT && (defined(RLIM_SAVED_CUR) || defined(RLIM_SAVED_MAX))
	    val == RLIM_SAVED_CUR || val == RLIM_SAVED_MAX ||
#endif
	    val == RL_U)
		goto inv_val;
 got_val:

#if HAVE_RLIMIT
	if (getrlimit(l->resource, &limit) < 0) {
#ifndef MKSH_SMALL
		kwarnf(KWF_BIERR | KWF_TWOMSG, l->name,
		    "limit could not be read, contact the mksh developers");
#endif
		/* some can't be read */
		limit.rlim_cur = RLIM_INFINITY;
		limit.rlim_max = RLIM_INFINITY;
	}
	if (how & SOFT)
		limit.rlim_cur = val;
	if (how & HARD)
		limit.rlim_max = val;
	if (!setrlimit(l->resource, &limit))
		return (0);
#else
	if (l->writable == Nee) {
	    /* check.t:ulimit-2 fails if we return 1 and/or do:
		kwarnf(KWF_BIERR | KWF_TWOMSG | KWF_NOERRNO,
		    Tread_only, l->name);
	    */
		return (0);
	}
	if (ulimit(l->wesource, val) != -1L)
		return (0);
#endif
	if (errno == EPERM)
		kwarnf0(KWF_BIERR | KWF_NOERRNO,
		    "%s exceeds allowable %s limit", v, l->name);
	else
		kwarnf0(KWF_BIERR, "%s: bad %s limit", v, l->name);
	return (1);
}

static void
print_ulimit(const struct limits *l, int how MKSH_A_UNUSED)
{
	RL_T val = (RL_T)0;
	char numbuf[NUMBUFSZ];
#if HAVE_RLIMIT
	struct rlimit limit;

	if (getrlimit(l->resource, &limit))
#else
	if ((val = ulimit(l->resource, 0)) < 0)
#endif
	    {
		shf_puts("unknown", shl_stdout);
		goto out;
	}
#if HAVE_RLIMIT
	if (how & SOFT)
		val = limit.rlim_cur;
	else
		val = limit.rlim_max;
#endif
	if (val == RL_U)
		shf_puts("unlimited", shl_stdout);
	else {
#if HAVE_RLIMIT
		val /= l->factor;
#elif defined(KSH_UL_GBRK)
		if (l->resource == KSH_UL_GBRK)
			val = (RL_T)(((size_t)val - (size_t)&etext) /
			    (size_t)1024);
#endif
		shf_puts(mbiTYPE_ISU(RL_T) ?
		    kuHfmt((kuH)val, FL_DEC, numbuf) :
		    ksHfmt((ksH)val, FL_DEC, numbuf), shl_stdout);
	}
 out:
	shf_putc('\n', shl_stdout);
}
