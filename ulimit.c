/*	$OpenBSD: c_ulimit.c,v 1.19 2013/11/28 10:33:37 sobrado Exp $	*/

/*-
 * Copyright (c) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009,
 *		 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017,
 *		 2019, 2020
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

__RCSID("$MirOS: src/bin/mksh/ulimit.c,v 1.3 2020/07/24 21:08:26 tg Exp $");

#define SOFT	0x1
#define HARD	0x2

#if HAVE_RLIMIT

#if !HAVE_RLIM_T
typedef unsigned long rlim_t;
#endif

/* Magic to divine the 'm' and 'v' limits */

#ifdef RLIMIT_AS
#if !defined(RLIMIT_VMEM) || (RLIMIT_VMEM == RLIMIT_AS) || \
    !defined(RLIMIT_RSS) || (RLIMIT_VMEM == RLIMIT_RSS)
#define ULIMIT_V_IS_AS
#elif defined(RLIMIT_VMEM)
#if !defined(RLIMIT_RSS) || (RLIMIT_RSS == RLIMIT_AS)
#define ULIMIT_V_IS_AS
#else
#define ULIMIT_V_IS_VMEM
#endif
#endif
#endif

#ifdef RLIMIT_RSS
#ifdef ULIMIT_V_IS_VMEM
#define ULIMIT_M_IS_RSS
#elif defined(RLIMIT_VMEM) && (RLIMIT_VMEM == RLIMIT_RSS)
#define ULIMIT_M_IS_VMEM
#else
#define ULIMIT_M_IS_RSS
#endif
#if defined(ULIMIT_M_IS_RSS) && defined(RLIMIT_AS) && \
    !defined(__APPLE__) && (RLIMIT_RSS == RLIMIT_AS)
/* On Mac OSX keep -m as -v alias for pkgsrc and other software expecting it */
#undef ULIMIT_M_IS_RSS
#endif
#endif

#if !defined(RLIMIT_AS) && !defined(ULIMIT_M_IS_VMEM) && defined(RLIMIT_VMEM)
#define ULIMIT_V_IS_VMEM
#endif

#if !defined(ULIMIT_V_IS_VMEM) && defined(RLIMIT_VMEM) && \
    (!defined(RLIMIT_RSS) || (defined(RLIMIT_AS) && (RLIMIT_RSS == RLIMIT_AS)))
#define ULIMIT_M_IS_VMEM
#endif

#if defined(ULIMIT_M_IS_VMEM) && defined(RLIMIT_AS) && \
    (RLIMIT_VMEM == RLIMIT_AS)
#undef ULIMIT_M_IS_VMEM
#endif

#if defined(ULIMIT_M_IS_RSS) && defined(ULIMIT_M_IS_VMEM)
# error nonsensical m ulimit
#endif

#if defined(ULIMIT_V_IS_VMEM) && defined(ULIMIT_V_IS_AS)
# error nonsensical v ulimit
#endif

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
#define KSH_UL_WFIL	true
#else
#define KSH_UL_WFIL	false
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
	bool writable;
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
		bool writable;				\
		char optchar;				\
		char name[sizeof(lname)];		\
	} rlimits_ ## lg = {				\
		lg, ls, lw, lopt, lname			\
	};
#endif
#include LIMITS_GEN

static void print_ulimit(const struct limits *, int);
static int set_ulimit(const struct limits *, const char *, int);

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
	bool all = false;

	while ((optc = ksh_getopt(wp, &builtin_opt, rlimits_opts)) != -1)
		switch (optc) {
		case ORD('H'):
			how = HARD;
			break;
		case ORD('S'):
			how = SOFT;
			break;
		case ORD('a'):
			all = true;
			break;
		case ORD('?'):
			bi_errorf("usage: ulimit [-%s] [value]", rlimits_opts);
			return (1);
		default:
			what = optc;
		}

	while (i < NELEM(rlimits)) {
		if (rlimits[i]->optchar == what)
			goto found;
		++i;
	}
#ifndef RLIMIT_CORE
	if (what == ORD('c'))
		/* silently accept */
		return 0;
#endif
	internal_warningf("ulimit: %c", what);
	return (1);
 found:
	if (wp[builtin_opt.optind]) {
		if (all || wp[builtin_opt.optind + 1]) {
			bi_errorf(Ttoo_many_args);
			return (1);
		}
		return (set_ulimit(rlimits[i], wp[builtin_opt.optind], how));
	}
	if (!all)
		print_ulimit(rlimits[i], how);
	else for (i = 0; i < NELEM(rlimits); ++i) {
		shprintf("-%c: %-20s  ", rlimits[i]->optchar, rlimits[i]->name);
		print_ulimit(rlimits[i], how);
	}
	return (0);
}

#if HAVE_RLIMIT
#define RL_T rlim_t
#define RL_U (rlim_t)RLIM_INFINITY
#else
#define RL_T long
#define RL_U LONG_MAX
#endif

static int
set_ulimit(const struct limits *l, const char *v, int how MKSH_A_UNUSED)
{
	RL_T val = (RL_T)0;
#if HAVE_RLIMIT
	struct rlimit limit;
#endif

	if (strcmp(v, "unlimited") == 0)
		val = RL_U;
	else {
		mksh_uari_t rval;

		if (!evaluate(v, (mksh_ari_t *)&rval, KSH_RETURN_ERROR, false))
			return (1);
		/*
		 * Avoid problems caused by typos that evaluate misses due
		 * to evaluating unset parameters to 0...
		 * If this causes problems, will have to add parameter to
		 * evaluate() to control if unset params are 0 or an error.
		 */
		if (!rval && !ctype(v[0], C_DIGIT)) {
			bi_errorf("invalid %s limit: %s", l->name, v);
			return (1);
		}
#if HAVE_RLIMIT
		val = (rlim_t)((rlim_t)rval * l->factor);
#else
		val = (RL_T)rval;
#endif
	}

#if HAVE_RLIMIT
	if (getrlimit(l->resource, &limit) < 0) {
#ifndef MKSH_SMALL
		bi_errorf("limit %s could not be read, contact the mksh developers: %s",
		    l->name, cstrerror(errno));
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
	if (l->writable == false) {
	    /* check.t:ulimit-2 fails if we return 1 and/or do:
		bi_errorf(Tf_ro, l->name);
	    */
		return (0);
	}
	if (ulimit(l->wesource, val) != -1L)
		return (0);
#endif
	if (errno == EPERM)
		bi_errorf("%s exceeds allowable %s limit", v, l->name);
	else
		bi_errorf("bad %s limit: %s", l->name, cstrerror(errno));
	return (1);
}

static void
print_ulimit(const struct limits *l, int how MKSH_A_UNUSED)
{
	RL_T val = (RL_T)0;
#if HAVE_RLIMIT
	struct rlimit limit;

	if (getrlimit(l->resource, &limit))
#else
	if ((val = ulimit(l->resource, 0)) < 0)
#endif
	    {
		shf_puts("unknown\n", shl_stdout);
		return;
	}
#if HAVE_RLIMIT
	if (how & SOFT)
		val = limit.rlim_cur;
	else if (how & HARD)
		val = limit.rlim_max;
#endif
	if (val == RL_U)
		shf_puts("unlimited\n", shl_stdout);
	else {
#if HAVE_RLIMIT
		val /= l->factor;
#elif defined(KSH_UL_GBRK)
		if (l->resource == KSH_UL_GBRK)
			val = (RL_T)(((size_t)val - (size_t)&etext) /
			    (size_t)1024);
#endif
		shprintf("%lu\n", (unsigned long)val);
	}
}
