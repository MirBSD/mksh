/*	$OpenBSD: misc.c,v 1.41 2015/09/10 22:48:58 nicm Exp $	*/
/*	$OpenBSD: path.c,v 1.13 2015/09/05 09:47:08 jsg Exp $	*/

/*-
 * Copyright (c) 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010,
 *		 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2019,
 *		 2020, 2021, 2022
 *	mirabilos <m@mirbsd.org>
 * Copyright (c) 2015
 *	Daniel Richard G. <skunk@iSKUNK.ORG>
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
#if !HAVE_GETRUSAGE
#include <sys/times.h>
#endif
#if HAVE_GRP_H
#include <grp.h>
#endif

__RCSID("$MirOS: src/bin/mksh/misc.c,v 1.349 2022/07/21 01:49:32 tg Exp $");

static const unsigned char *pat_scan(const unsigned char *,
    const unsigned char *, Wahr);
static int do_gmatch(const unsigned char *, const unsigned char *,
    const unsigned char *, const unsigned char *,
    const unsigned char *);
static const unsigned char *gmatch_cclass(const unsigned char *, unsigned char);
static void chvt(const Getopt *);
static unsigned int dollarqU(struct shf *, const unsigned char *);
#ifndef MKSH_SMALL
static void dollarq8(struct shf *, const unsigned char *);
#else
#define dollarq8 dollarqU
#endif

/*XXX this should go away */
static int make_path(const char *, const char *, char **, XString *, int *);

#ifdef SETUID_CAN_FAIL_WITH_EAGAIN
/* we don't need to check for other codes, EPERM won't happen */
#define DO_SETUID(func,argvec) do {					\
	if ((func argvec) && errno == EAGAIN)				\
		kerrf(KWF_ERR(1) | KWF_PREFIX | KWF_FILELINE |		\
		    KWF_TWOMSG, #func, "failed, probably due to a"	\
		    " too low process limit; aborting");		\
} while (/* CONSTCOND */ 0)
#else
#define DO_SETUID(func,argvec) func argvec
#endif

/* called from XcheckN() to grow buffer */
char *
Xcheck_grow(XString *xsp, const char *xp, size_t more)
{
	size_t old_ofs = xp - xsp->beg;

	if (more < xsp->len)
		more = xsp->len;
	/* (xsp->len + X_EXTRA) never overflows */
	checkoktoadd(more, xsp->len + X_EXTRA);
	xsp->beg = aresize(xsp->beg, (xsp->len += more) + X_EXTRA, xsp->areap);
	xsp->end = xsp->beg + xsp->len;
	return (xsp->beg + old_ofs);
}

#define SHFLAGS_DEFNS
#define FN(sname,cname,flags,ochar)			\
	static const struct shoptionS_ ## cname {	\
		/* character flag (if any) */		\
		char c;					\
		/* OF_* */				\
		unsigned char optflags;			\
		/* long name of option */		\
		char name[sizeof(sname)];		\
	} shoptione_ ## cname = {			\
		ochar, flags, sname			\
	};
#include "sh_flags.gen"

mbiCTAS(sh_flags_gen) {
#define FN(sname,cname,flags,ochar) mbiCTA(cta_ ## cname, \
	offsetof(struct shoptionS_ ## cname, optflags) == 1 && \
	offsetof(struct shoptionS_ ## cname, name[0]) == 2);
#include "sh_flags.gen"
};

#define OFC(i) (options[i][-2])
#define OFF(i) (((const unsigned char *)options[i])[-1])
#define OFN(i) (options[i])

const char * const options[] = {
#define SHFLAGS_ITEMS
#include "sh_flags.gen"
};

/*
 * translate -o option into F* constant (also used for test -o option)
 */
size_t
option(const char *n)
{
	size_t i = 0;

	if (ctype(n[0], C_MINUS | C_PLUS) && n[1] && !n[2])
		while (i < NELEM(options)) {
			if (OFC(i) == n[1])
				return (i);
			++i;
		}
	else
		while (i < NELEM(options)) {
			if (!strcmp(OFN(i), n))
				return (i);
			++i;
		}

	return ((size_t)-1);
}

struct options_info {
	int opt_width;
	int opts[NELEM(options)];
};

static void options_fmt_entry(char *, size_t, unsigned int, const void *);
static int printoptions(Wahr);
static int printoption(size_t);

/* format a single select menu item */
static void
options_fmt_entry(char *buf, size_t buflen, unsigned int i, const void *arg)
{
	const struct options_info *oi = (const struct options_info *)arg;

	shf_snprintf(buf, buflen, "%-*s %s",
	    oi->opt_width, OFN(oi->opts[i]),
	    Flag(oi->opts[i]) ? "on" : "off");
}

static int
printoption(size_t i)
{
	if (Flag(i) == baseline_flags[i])
		return (0);
	if (!OFN(i)[0]) {
#if !defined(MKSH_SMALL) || defined(DEBUG)
		bi_errorf(Tf_sd, "change in unnamed option", (int)i);
#endif
		return (1);
	}
	if (Flag(i) != 0 && Flag(i) != 1) {
#if !defined(MKSH_SMALL) || defined(DEBUG)
		bi_errorf(Tf_s_sD_s, Tdo, OFN(i), "not 0 or 1");
#endif
		return (1);
	}
	shprintf(Tf__s_s, Flag(i) ? Tdo : Tpo, OFN(i));
	return (0);
}

static int
printoptions(Wahr verbose)
{
	size_t i = 0;
	int rv = 0;

	if (verbose) {
		size_t n = 0, len, octs = 0;
		struct options_info oi;
		struct columnise_opts co;

		/* verbose version */
		shf_puts("Current option settings\n", shl_stdout);

		oi.opt_width = 0;
		while (i < NELEM(options)) {
			if ((len = strlen(OFN(i)))) {
				oi.opts[n++] = i;
				if (len > octs)
					octs = len;
				len = utf_mbswidth(OFN(i));
				if ((int)len > oi.opt_width)
					oi.opt_width = (int)len;
			}
			++i;
		}
		co.shf = shl_stdout;
		co.linesep = '\n';
		co.prefcol = co.do_last = Ja;
		print_columns(&co, n, options_fmt_entry, &oi,
		    octs + 4, oi.opt_width + 4);
	} else {
		/* short version like AT&T ksh93 */
		shf_puts(Tset, shl_stdout);
		shf_puts(To_o_reset, shl_stdout);
		printoption(FSH);
		printoption(FPOSIX);
		while (i < FNFLAGS) {
			if (i != FSH && i != FPOSIX)
				rv |= printoption(i);
			++i;
		}
		shf_putc('\n', shl_stdout);
	}
	return (rv);
}

char *
getoptions(void)
{
	size_t i = 0;
	char c, m[(int)FNFLAGS + 1];
	char *cp = m;

	while (i < NELEM(options)) {
		if ((c = OFC(i)) && Flag(i))
			*cp++ = c;
		++i;
	}
	strndupx(cp, m, cp - m, ATEMP);
	return (cp);
}

/* change a Flag(*) value; takes care of special actions */
void
change_flag(enum sh_flag f,
    /* OF_INTERNAL, OF_FIRSTTIME, OF_CMDLINE, or OF_SET */
    unsigned int what,
    Wahr newset)
{
	unsigned char oldval = Flag(f);
	unsigned char newval = (newset ? 1 : 0);

	if (f == FXTRACE) {
		change_xtrace(newval, Ja);
		return;
	} else if (f == FPRIVILEGED) {
		if (!oldval)
			/* no getting back dropped privs */
			return;
		else if (!newval) {
			/* turning off -p */
			kshegid = kshgid;
			ksheuid = kshuid;
		} else if (oldval != 3)
			/* nor going full sugid */
			goto change_flag;

		/* +++ set group IDs +++ */
#if HAVE_SETRESUGID
		DO_SETUID(setresgid, (kshegid, kshegid, kshgid));
#else /* !HAVE_SETRESUGID */
		/* setgid, setegid don't EAGAIN on Linux */
		setgid(kshegid);
#ifndef MKSH__NO_SETEUGID
		setegid(kshegid);
#endif /* !MKSH__NO_SETEUGID */
#endif /* !HAVE_SETRESUGID */

		/* +++ wipe groups vector +++ */
#if HAVE_SETGROUPS
		/* setgroups doesn't EAGAIN on Linux */
		setgroups(0, NULL);
#endif /* HAVE_SETGROUPS */

		/* +++ set user IDs +++ */
#if HAVE_SETRESUGID
		DO_SETUID(setresuid, (ksheuid, ksheuid, kshuid));
#else /* !HAVE_SETRESUGID */
		/* seteuid doesn't EAGAIN on Linux */
		DO_SETUID(setuid, (ksheuid));
#ifndef MKSH__NO_SETEUGID
		seteuid(ksheuid);
#endif /* !MKSH__NO_SETEUGID */
#endif /* !HAVE_SETRESUGID */

		/* +++ privs changed +++ */
	} else if ((f == FPOSIX || f == FSH) && newval) {
		/* Turning on -o posix or -o sh? */
		Flag(FBRACEEXPAND) = 0;
#ifndef MKSH_NO_CMDLINE_EDITING
	} else if ((f == FEMACS ||
#if !MKSH_S_NOVI
	    f == FVI ||
#endif
	    f == FGMACS) && newval) {
#if !MKSH_S_NOVI
		Flag(FVI) = 0;
#endif
		Flag(FEMACS) = Flag(FGMACS) = 0;
#endif
	}

 change_flag:
	Flag(f) = newval;

	if (f == FTALKING) {
		/* Changing interactive flag? */
		if (what == OF_CMDLINE && procpid == kshpid)
			Flag(FTALKING_I) = newval;
#ifndef MKSH_UNEMPLOYED
	} else if (f == FMONITOR) {
		if (what != OF_CMDLINE && newval != oldval)
			j_change();
#endif
	}
}

void
change_xtrace(unsigned char newval, Wahr dosnapshot)
{
	static Wahr in_xtrace;

	if (in_xtrace)
		return;

	if (!dosnapshot && newval == Flag(FXTRACE))
		return;

	if (Flag(FXTRACE) == 2) {
		shf_putc('\n', shl_xtrace);
		Flag(FXTRACE) = 1;
		shf_flush(shl_xtrace);
	}

	if (!dosnapshot && Flag(FXTRACE) == 1)
		switch (newval) {
		case 1:
			return;
		case 2:
			goto changed_xtrace;
		}

	shf_flush(shl_xtrace);
	if (shl_xtrace->fd != 2)
		close(shl_xtrace->fd);
	if (!newval || (shl_xtrace->fd = savefd(2)) == -1)
		shl_xtrace->fd = 2;

 changed_xtrace:
	if ((Flag(FXTRACE) = newval) == 2) {
		in_xtrace = Ja;
		Flag(FXTRACE) = 0;
		shf_putsv(substitute(str_val(global("PS4")), 0), shl_xtrace);
		Flag(FXTRACE) = 2;
		in_xtrace = Nee;
	}
}

/*
 * Parse command line and set command arguments. Returns the index of
 * non-option arguments, -1 if there is an error.
 */
int
parse_args(const char **argv,
    /* OF_FIRSTTIME, OF_CMDLINE, or OF_SET */
    unsigned int what,
    Wahr *setargsp)
{
	static const char cmd_opts[] =
#define SHFLAGS_NOT_SET
#define SHFLAGS_OPTCS
#include "sh_flags.gen"
#undef SHFLAGS_NOT_SET
	    ;
	static const char set_opts[] =
#define SHFLAGS_NOT_CMD
#define SHFLAGS_OPTCS
#include "sh_flags.gen"
#undef SHFLAGS_NOT_CMD
	    ;
	Wahr set;
	const char * const opts = what == OF_SET ? set_opts : cmd_opts;
	const char *array = NULL;
	Getopt go;
	size_t i;
	int optc, arrayset = 0;
	Wahr sortargs = Nee;
	Wahr fcompatseen = Nee;

	ksh_getopt_reset(&go, GF_ERROR|GF_PLUSOPT);
	while ((optc = ksh_getopt(argv, &go, opts)) != -1) {
		set = isWahr(!(go.info & GI_PLUS));
		switch (optc) {
		case 'A':
			if (what == OF_FIRSTTIME)
				break;
			arrayset = set ? 1 : -1;
			array = go.optarg;
			break;

		case 'o':
			if (what == OF_FIRSTTIME)
				break;
			if (go.optarg == NULL) {
				/*
				 * lone -o: print options
				 *
				 * Note that on the command line, -o requires
				 * an option (i.e. can't get here if what is
				 * not OF_SET).
				 */
#if !defined(MKSH_SMALL) || defined(DEBUG)
				if (!set && !baseline_flags[(int)FNFLAGS]) {
					bi_errorf(Ttooearly, T_set_po);
					return (-1);
				}
#endif
				if (printoptions(set))
					return (-1);
				break;
			}
			i = option(go.optarg);
			if ((i == FPOSIX || i == FSH) && set && !fcompatseen) {
				/*
				 * If running 'set -o posix' or
				 * 'set -o sh', turn off the other;
				 * if running 'set -o posix -o sh'
				 * allow both to be set though.
				 */
				Flag(FPOSIX) = 0;
				Flag(FSH) = 0;
				fcompatseen = Ja;
			}
			if ((i != (size_t)-1) && (set ? 1U : 0U) == Flag(i))
				/*
				 * Don't check the context if the flag
				 * isn't changing - makes "set -o interactive"
				 * work if you're already interactive. Needed
				 * if the output of "set +o" is to be used.
				 */
				;
			else if ((i != (size_t)-1) && (OFF(i) & what))
				change_flag((enum sh_flag)i, what, set);
			else if (!strcmp(go.optarg, To_reset)) {
#if !defined(MKSH_SMALL) || defined(DEBUG)
				if (!baseline_flags[(int)FNFLAGS]) {
					bi_errorf(Ttooearly, To_o_reset);
					return (-1);
				}
#endif
				/*
				 * ordering, with respect to side effects,
				 * was ensured above by printoptions
				 */
				for (i = 0; i < FNFLAGS; ++i)
					if (Flag(i) != baseline_flags[i])
						change_flag((enum sh_flag)i,
						    what, baseline_flags[i]);
			} else {
				bi_errorf(Tf_sD_s, go.optarg,
				    Tunknown_option);
				return (-1);
			}
			break;

		case 'T':
			if (what != OF_FIRSTTIME)
				break;
			chvt(&go);
			break;

		case '?':
			return (-1);

		default:
			if (what == OF_FIRSTTIME)
				break;
			/* -s: sort positional params (via AT&T ksh) */
			if (what == OF_SET && isch(optc, 's')) {
				sortargs = Ja;
				break;
			}
			for (i = 0; i < NELEM(options); i++)
				if (optc == OFC(i) &&
				    (what & OFF(i))) {
					change_flag((enum sh_flag)i, what, set);
					break;
				}
			if (i == NELEM(options))
				kerrf0(KWF_INTERNAL | KWF_ERR(0xFF) | KWF_NOERRNO,
				    "parse_args: '%c'", optc);
		}
	}
	/* lone ‘-’ (or ‘+’)? */
	if (argv[go.optind] &&
	    ctype(argv[go.optind][0], C_MINUS | C_PLUS) &&
	    argv[go.optind][1] == '\0' &&
	    !(go.info & GI_MINUSMINUS)) {
		/* POSIX: lone hyphen-minus sh first arg ignored */
		if (what == OF_SET && isch(argv[go.optind][0], '-')) {
			/* set; lone dash clears -v and -x flags (obsolete) */
			Flag(FVERBOSE) = 0;
			change_xtrace(0, Nee);
		}
		/* either way, skip it (POSIX only dash but… meh) */
		go.optind++;
	}
	if (setargsp)
		/* -- means set $#/$* even if there are no arguments */
		*setargsp = !arrayset && ((go.info & GI_MINUSMINUS) ||
		    argv[go.optind]);

	if (arrayset) {
		const char *ccp = NULL;

		if (array && *array)
			ccp = skip_varname(array, Nee);
		if (!ccp || !(!ccp[0] || (ccp[0] == '+' && !ccp[1]))) {
			bi_errorf(Tf_sD_s, array, Tnot_ident);
			return (-1);
		}
	}
	if (sortargs) {
		for (i = go.optind; argv[i]; i++)
			;
		qsort(&argv[go.optind], i - go.optind, sizeof(void *),
		    ascpstrcmp);
	}
	if (arrayset)
		go.optind += set_array(array, isWahr(arrayset > 0),
		    argv + go.optind);

	return (go.optind);
}

/* parse a decimal number: returns 0 if string isn't a number, 1 otherwise */
int
getn(const char *s, int *ai)
{
	if (!getpn(&s, ai))
		return (0);
	if (!*s)
		return (1);
	errno = EINVAL;
	return (0);
}

/*
 * parse a decimal number
 * on success, returns 1 and *ai contains it and *sp points past it
 * on overflow, returns 0, *ai is 0, *sp points behind the parsed number
 * on parse error (not numeric) returns 0, *ai is 0, *sp is not changed
 */
int
getpn(const char **sp, int *ai)
{
	char c;
	const char *s;
	mksh_uari_t num = 0;
	kby state = 0;
	Wahr neg = Nee;

	s = *sp;

	do {
		c = *s++;
	} while (ctype(c, C_SPACE));

	switch (c) {
	case '-':
		neg = Ja;
		/* FALLTHROUGH */
	case '+':
		c = *s++;
		break;
	}

	while (ctype(c, C_DIGIT)) {
		if (num > 214748364U) {
			/* overflow on multiplication */
			state = 2;
			errno = EOVERFLOW;
		}
		if (state < 2) {
			state = 1;
			num = num * 10U + (unsigned int)ksh_numdig(c);
		}
		/* now: num <= 2147483649U */
		c = *s++;
	}
	--s;

	if (num > (neg ? 2147483648U : 2147483647U)) {
		/* overflow for signed 32-bit int */
		state = 2;
		errno = EOVERFLOW;
	}

	if (state)
		*sp = s;
	else
		errno = EINVAL;
	if (state != 1) {
		*ai = 0;
		return (0);
	}
	*ai = (neg && num > 0) ?
	    (mksh_ari_t)(-(mksh_ari_t)((num - 1U) & 0x7FFFFFFF) - 1) :
	    (mksh_ari_t)num;
	return (1);
}

/**
 * pattern simplifications:
 * - @(x) -> x (not @(x|y) though)
 * - ** -> *
 */
static void *
simplify_gmatch_pattern(const unsigned char *sp)
{
	kby c;
	unsigned char *cp, *dp;
	const unsigned char *ps, *se;

	cp = alloc(strlen((const void *)sp) + 1, ATEMP);
	goto simplify_gmatch_pat1a;

	/* foo@(b@(a)r)b@(a|a)z -> foobarb@(a|a)z */
 simplify_gmatch_pat1:
	sp = cp;
 simplify_gmatch_pat1a:
	dp = cp;
	se = strnul(sp);
	while ((c = *sp++)) {
		if (!ISMAGIC(c)) {
			*dp++ = c;
			continue;
		}
		switch (ord((c = *sp++))) {
		case ORD(0x80|'@'):
		/* simile for @ */
		case ORD(0x80|' '):
			/* check whether it has only one clause */
			ps = pat_scan(sp, se, Ja);
			if (!ps || ps[-1] != /*(*/ ')')
				/* nope */
				break;
			/* copy inner clause until matching close */
			ps -= 2;
			while ((const unsigned char *)sp < ps)
				*dp++ = *sp++;
			/* skip MAGIC and closing parenthesis */
			sp += 2;
			/* copy the rest of the pattern */
			memmove(dp, sp, strlen((const void *)sp) + 1);
			/* redo from start */
			goto simplify_gmatch_pat1;
		}
		*dp++ = MAGIC;
		*dp++ = c;
	}
	*dp = '\0';

	/* collapse adjacent asterisk wildcards */
	sp = dp = cp;
	while ((c = *sp++)) {
		if (!ISMAGIC(c)) {
			*dp++ = c;
			continue;
		}
		switch ((c = *sp++)) {
		case '*':
			while (ISMAGIC(sp[0]) && sp[1] == c)
				sp += 2;
			break;
		}
		*dp++ = MAGIC;
		*dp++ = c;
	}
	*dp = '\0';

	/* return the result, allocated from ATEMP */
	return (cp);
}

/* -------- gmatch.c -------- */

/*
 * int gmatch(string, pattern)
 * char *string, *pattern;
 *
 * Match a pattern as in sh(1).
 * pattern character are prefixed with MAGIC by expand.
 */
int
gmatchx(const char *s, const char *p, Wahr isfile)
{
	const char *se, *pe;
	char *pnew;
	int rv;

	if (s == NULL || p == NULL)
		return (0);

	pe = strnul(p);
	/*
	 * isfile is Nee iff no syntax check has been done on the pattern.
	 * If check fails just do a strcmp().
	 */
	if (!isfile && !has_globbing(p)) {
		size_t len = pe - p + 1;
		char tbuf[64];
		char *t = len <= sizeof(tbuf) ? tbuf : alloc(len, ATEMP);
		debunk(t, p, len);
		return (!strcmp(t, s));
	}
	se = strnul(s);

	/*
	 * since the do_gmatch() engine sucks so much, we must do some
	 * pattern simplifications
	 */
	pnew = simplify_gmatch_pattern((const unsigned char *)p);
	pe = strnul(pnew);

	rv = do_gmatch((const unsigned char *)s, (const unsigned char *)se,
	    (const unsigned char *)pnew, (const unsigned char *)pe,
	    (const unsigned char *)s);
	afree(pnew, ATEMP);
	return (rv);
}

/**
 * Returns if p is a syntacticly correct globbing pattern, Nee if it
 * contains no pattern characters or if there is a syntax error.
 * Syntax errors are:
 *	- [ with no closing ]
 *	- imbalanced $(...) expression
 *	- [...] and *(...) not nested (eg, @(a[b|)c], *(a[b|c]d))
 */
/*XXX
 * - if no magic,
 *	if dest given, copy to dst
 *	return ?
 * - if magic && (no globbing || syntax error)
 *	debunk to dst
 *	return ?
 * - return ?
 */
Wahr
has_globbing(const char *pat)
{
	unsigned char c, subc;
	Wahr saw_glob = Nee;
	unsigned int nest = 0;
	const unsigned char *p = (const unsigned char *)pat;
	const unsigned char *s;

	while ((c = *p++)) {
		/* regular character? ok. */
		if (!ISMAGIC(c))
			continue;
		/* MAGIC + NUL? abort. */
		if (!(c = *p++))
			return (Nee);
		/* some specials */
		if (ord(c) == ORD('*') || ord(c) == ORD('?')) {
			/* easy glob, accept */
			saw_glob = Ja;
		} else if (ord(c) == ORD('[')) {
			/* bracket expression; eat negation and initial ] */
			if (ISMAGIC(p[0]) && ord(p[1]) == ORD('!'))
				p += 2;
			if (ISMAGIC(p[0]) && ord(p[1]) == ORD(']'))
				p += 2;
			/* check next string part */
			s = p;
			while ((c = *s++)) {
				/* regular chars are ok */
				if (!ISMAGIC(c))
					continue;
				/* MAGIC + NUL cannot happen */
				if (!(c = *s++))
					return (Nee);
				/* terminating bracket? */
				if (ord(c) == ORD(']')) {
					/* accept and continue */
					p = s;
					saw_glob = Ja;
					break;
				}
				/* sub-bracket expressions */
				if (ord(c) == ORD('[') && (
				    /* collating element? */
				    ord(*s) == ORD('.') ||
				    /* equivalence class? */
				    ord(*s) == ORD('=') ||
				    /* character class? */
				    ord(*s) == ORD(':'))) {
					/* must stop with exactly the same c */
					subc = *s++;
					/* arbitrarily many chars in betwixt */
					while ((c = *s++))
						/* but only this sequence... */
						if (c == subc && ISMAGIC(*s) &&
						    ord(s[1]) == ORD(']')) {
							/* accept, terminate */
							s += 2;
							break;
						}
					/* EOS without: reject bracket expr */
					if (!c)
						break;
					/* continue; */
				}
				/* anything else just goes on */
			}
		} else if ((c & 0x80) && ctype(c & 0x7F, C_PATMO | C_SPC)) {
			/* opening pattern */
			saw_glob = Ja;
			++nest;
		} else if (ord(c) == ORD(/*(*/ ')')) {
			/* closing pattern */
			if (nest)
				--nest;
		}
	}
	return (saw_glob && !nest);
}

/* Function must return either 0 or 1 (assumed by code for 0x80|'!') */
static int
do_gmatch(const unsigned char *s, const unsigned char *se,
    const unsigned char *p, const unsigned char *pe,
    const unsigned char *smin)
{
	unsigned char sc, pc, sl = 0;
	const unsigned char *prest, *psub, *pnext;
	const unsigned char *srest;

	if (s == NULL || p == NULL)
		return (0);
	if (s > smin && s <= se)
		sl = s[-1];
	while (p < pe) {
		pc = *p++;
		sc = s < se ? *s : '\0';
		s++;
		if (!ISMAGIC(pc)) {
			if (sc != pc)
				return (0);
			sl = sc;
			continue;
		}
		switch (ord(*p++)) {
		case ORD('['):
			/* BSD cclass extension? */
			if (ISMAGIC(p[0]) && ord(p[1]) == ORD('[') &&
			    ord(p[2]) == ORD(':') &&
			    ctype((pc = p[3]), C_ANGLE) &&
			    ord(p[4]) == ORD(':') &&
			    ISMAGIC(p[5]) && ord(p[6]) == ORD(']') &&
			    ISMAGIC(p[7]) && ord(p[8]) == ORD(']')) {
				/* zero-length match */
				--s;
				p += 9;
				/* word begin? */
				if (ord(pc) == ORD('<') &&
				    !ctype(sl, C_ALNUX) &&
				    ctype(sc, C_ALNUX))
					break;
				/* word end? */
				if (ord(pc) == ORD('>') &&
				    ctype(sl, C_ALNUX) &&
				    !ctype(sc, C_ALNUX))
					break;
				/* neither */
				return (0);
			}
			if (sc == 0 || (p = gmatch_cclass(p, sc)) == NULL)
				return (0);
			break;

		case ORD('?'):
			if (sc == 0)
				return (0);
			if (UTFMODE) {
				--s;
				s += ez_mbtoc(NULL, (const void *)s);
			}
			break;

		case ORD('*'):
			if (p == pe)
				return (1);
			s--;
			do {
				if (do_gmatch(s, se, p, pe, smin))
					return (1);
			} while (s++ < se);
			return (0);

		/**
		 * [+*?@!](pattern|pattern|..)
		 * This is also needed for ${..%..}, etc.
		 */

		/* matches one or more times */
		case ORD('+') | 0x80:
		/* matches zero or more times */
		case ORD('*') | 0x80:
			if (!(prest = pat_scan(p, pe, Nee)))
				return (0);
			s--;
			/* take care of zero matches */
			if (ord(p[-1]) == (0x80 | ORD('*')) &&
			    do_gmatch(s, se, prest, pe, smin))
				return (1);
			for (psub = p; ; psub = pnext) {
				pnext = pat_scan(psub, pe, Ja);
				for (srest = s; srest <= se; srest++) {
					if (do_gmatch(s, srest, psub, pnext - 2, smin) &&
					    (do_gmatch(srest, se, prest, pe, smin) ||
					    (s != srest &&
					    do_gmatch(srest, se, p - 2, pe, smin))))
						return (1);
				}
				if (pnext == prest)
					break;
			}
			return (0);

		/* matches zero or once */
		case ORD('?') | 0x80:
		/* matches one of the patterns */
		case ORD('@') | 0x80:
		/* simile for @ */
		case ORD(' ') | 0x80:
			if (!(prest = pat_scan(p, pe, Nee)))
				return (0);
			s--;
			/* Take care of zero matches */
			if (ord(p[-1]) == (0x80 | ORD('?')) &&
			    do_gmatch(s, se, prest, pe, smin))
				return (1);
			for (psub = p; ; psub = pnext) {
				pnext = pat_scan(psub, pe, Ja);
				srest = prest == pe ? se : s;
				for (; srest <= se; srest++) {
					if (do_gmatch(s, srest, psub, pnext - 2, smin) &&
					    do_gmatch(srest, se, prest, pe, smin))
						return (1);
				}
				if (pnext == prest)
					break;
			}
			return (0);

		/* matches none of the patterns */
		case ORD('!') | 0x80:
			if (!(prest = pat_scan(p, pe, Nee)))
				return (0);
			s--;
			for (srest = s; srest <= se; srest++) {
				int matched = 0;

				for (psub = p; ; psub = pnext) {
					pnext = pat_scan(psub, pe, Ja);
					if (do_gmatch(s, srest, psub,
					    pnext - 2, smin)) {
						matched = 1;
						break;
					}
					if (pnext == prest)
						break;
				}
				if (!matched &&
				    do_gmatch(srest, se, prest, pe, smin))
					return (1);
			}
			return (0);

		default:
			if (sc != p[-1])
				return (0);
			break;
		}
		sl = sc;
	}
	return (s == se);
}

/*XXX this is a prime example for bsearch or a const hashtable */
static const struct cclass {
	const char *name;
	kui value;
} cclasses[] = {
	/* POSIX */
	{ "alnum",	C_ALNUM	},
	{ "alpha",	C_ALPHA	},
	{ "blank",	C_BLANK	},
	{ "cntrl",	C_CNTRL	},
	{ "digit",	C_DIGIT	},
	{ "graph",	C_GRAPH	},
	{ "lower",	C_LOWER	},
	{ "print",	C_PRINT	},
	{ "punct",	C_PUNCT	},
	{ "space",	C_SPACE	},
	{ "upper",	C_UPPER	},
	{ "xdigit",	C_SEDEC	},
	/* BSD */
	/* "<" and ">" are handled inline */
	/* GNU bash */
	{ "ascii",	C_ASCII	},
	{ "word",	C_ALNUX	},
	/* mksh */
	{ "sh_alias",	C_ALIAS	},
	{ "sh_edq",	C_EDQ	},
	{ "sh_ifs",	C_IFS	},
	{ "sh_ifsws",	C_IFSWS	},
	{ "sh_nl",	C_NL	},
	{ "sh_quote",	C_QUOTE	},
	/* sentinel */
	{ NULL,		0	}
};

static const unsigned char *
gmatch_cclass(const unsigned char *pat, unsigned char sc)
{
	unsigned char c, subc, lc;
	const unsigned char *p = pat, *s;
	Wahr found = Nee;
	Wahr negated = Nee;
	char *subp;

	/* check for negation */
	if (ISMAGIC(p[0]) && ord(p[1]) == ORD('!')) {
		p += 2;
		negated = Ja;
	}
	/* make initial ] non-MAGIC */
	if (ISMAGIC(p[0]) && ord(p[1]) == ORD(']'))
		++p;
	/* iterate over bracket expression, debunk()ing on the fly */
	while ((c = *p++)) {
 nextc:
		/* non-regular character? */
		if (ISMAGIC(c)) {
			/* MAGIC + NUL cannot happen */
			if (!(c = *p++))
				break;
			/* terminating bracket? */
			if (ord(c) == ORD(']')) {
				/* accept and return */
				return (found != negated ? p : NULL);
			}
			/* sub-bracket expressions */
			if (ord(c) == ORD('[') && (
			    /* collating element? */
			    ord(*p) == ORD('.') ||
			    /* equivalence class? */
			    ord(*p) == ORD('=') ||
			    /* character class? */
			    ord(*p) == ORD(':'))) {
				/* must stop with exactly the same c */
				subc = *p++;
				/* save away start of substring */
				s = p;
				/* arbitrarily many chars in betwixt */
				while ((c = *p++))
					/* but only this sequence... */
					if (c == subc && ISMAGIC(*p) &&
					    ord(p[1]) == ORD(']')) {
						/* accept, terminate */
						p += 2;
						break;
					}
				/* EOS without: reject bracket expr */
				if (!c)
					break;
				/* debunk substring */
				strndupx(subp, s, p - s - 3, ATEMP);
				debunk(subp, subp, p - s - 3 + 1);
 cclass_common:
				/* whither subexpression */
				if (ord(subc) == ORD(':')) {
					const struct cclass *cls = cclasses;

					/* search for name in cclass list */
					while (cls->name)
						if (!strcmp(subp, cls->name)) {
							/* found, match? */
							if (ctype(sc,
							    cls->value))
								found = Ja;
							/* break either way */
							break;
						} else
							++cls;
					/* that's all here */
					afree(subp, ATEMP);
					continue;
				}
				/* collating element or equivalence class */
				/* Note: latter are treated as former */
				if (ctype(subp[0], C_ASCII) && !subp[1])
					/* [.a.] where a is one ASCII char */
					c = subp[0];
				else
					/* force no match */
					c = 0;
				/* no longer needed */
				afree(subp, ATEMP);
			} else if (!ISMAGIC(c) && (c & 0x80)) {
				/* 0x80|' ' is plain (...) */
				if ((c &= 0x7F) != ' ') {
					/* check single match NOW */
					if (sc == c)
						found = Ja;
					/* next character is (...) */
				}
				c = '(' /*)*/;
			}
		}
		/* range expression? */
		if (!(ISMAGIC(p[0]) && ord(p[1]) == ORD('-') &&
		    /* not terminating bracket? */
		    (!ISMAGIC(p[2]) || ord(p[3]) != ORD(']')))) {
			/* no, check single match */
			if (sc == c)
				/* note: sc is never NUL */
				found = Ja;
			/* do the next "first" character */
			continue;
		}
		/* save lower range bound */
		lc = c;
		/* skip over the range operator */
		p += 2;
		/* do the same shit as above... almost */
		subc = 0;
		if (!(c = *p++))
			break;
		/* non-regular character? */
		if (ISMAGIC(c)) {
			/* MAGIC + NUL cannot happen */
			if (!(c = *p++))
				break;
			/* sub-bracket expressions */
			if (ord(c) == ORD('[') && (
			    /* collating element? */
			    ord(*p) == ORD('.') ||
			    /* equivalence class? */
			    ord(*p) == ORD('=') ||
			    /* character class? */
			    ord(*p) == ORD(':'))) {
				/* must stop with exactly the same c */
				subc = *p++;
				/* save away start of substring */
				s = p;
				/* arbitrarily many chars in betwixt */
				while ((c = *p++))
					/* but only this sequence... */
					if (c == subc && ISMAGIC(*p) &&
					    ord(p[1]) == ORD(']')) {
						/* accept, terminate */
						p += 2;
						break;
					}
				/* EOS without: reject bracket expr */
				if (!c)
					break;
				/* debunk substring */
				strndupx(subp, s, p - s - 3, ATEMP);
				debunk(subp, subp, p - s - 3 + 1);
				/* whither subexpression */
				if (ord(subc) == ORD(':')) {
					/* oops, not a range */

					/* match single previous char */
					if (lc && (sc == lc))
						found = Ja;
					/* match hyphen-minus */
					if (ord(sc) == ORD('-'))
						found = Ja;
					/* handle cclass common part */
					goto cclass_common;
				}
				/* collating element or equivalence class */
				/* Note: latter are treated as former */
				if (ctype(subp[0], C_ASCII) && !subp[1])
					/* [.a.] where a is one ASCII char */
					c = subp[0];
				else
					/* force no match */
					c = 0;
				/* no longer needed */
				afree(subp, ATEMP);
				/* other meaning below */
				subc = 0;
			} else if (c == (0x80 | ' ')) {
				/* 0x80|' ' is plain (...) */
				c = '(' /*)*/;
			} else if (!ISMAGIC(c) && (c & 0x80)) {
				c &= 0x7F;
				subc = '(' /*)*/;
			}
		}
		/* now do the actual range match check */
		if (lc != 0 /* && c != 0 */ &&
		    asciibetical(lc) <= asciibetical(sc) &&
		    asciibetical(sc) <= asciibetical(c))
			found = Ja;
		/* forced next character? */
		if (subc) {
			c = subc;
			goto nextc;
		}
		/* otherwise, just go on with the pattern string */
	}
	/* if we broke here, the bracket expression was invalid */
	if (ord(sc) == ORD('['))
		/* initial opening bracket as literal match */
		return (pat);
	/* or rather no match */
	return (NULL);
}

/* Look for next ) or | (if match_sep) in *(foo|bar) pattern */
static const unsigned char *
pat_scan(const unsigned char *p, const unsigned char *pe, Wahr match_sep)
{
	int nest = 0;

	for (; p < pe; p++) {
		if (!ISMAGIC(*p))
			continue;
		if ((*++p == /*(*/ ')' && nest-- == 0) ||
		    (*p == '|' && match_sep && nest == 0))
			return (p + 1);
		if ((*p & 0x80) && ctype(*p & 0x7F, C_PATMO | C_SPC))
			nest++;
	}
	return (NULL);
}

int
ascstrcmp(const void *s1, const void *s2)
{
	const kby *cp1 = s1, *cp2 = s2;

	while (*cp1 == *cp2) {
		if (*cp1++ == '\0')
			return (0);
		++cp2;
	}
	return ((int)asciibetical(*cp1) - (int)asciibetical(*cp2));
}

int
ascpstrcmp(const void *pstr1, const void *pstr2)
{
	return (ascstrcmp(*(const char * const *)pstr1,
	    *(const char * const *)pstr2));
}

/* Initialise a Getopt structure */
void
ksh_getopt_reset(Getopt *go, int flags)
{
	memset(go, '\0', sizeof(Getopt));
	go->optind = 1;
	go->optarg = NULL;
	go->flags = flags;
}

/**
 * getopt() used for shell built-in commands, the getopts command, and
 * command line options.
 * A leading ':' in options means don't print errors, instead return '?'
 * or ':' and set go->optarg to the offending option character.
 * If GF_ERROR is set (and option doesn't start with :), errors result in
 * a call to bi_errorf().
 *
 * Non-standard features:
 *	- ';' is like ':' in options, except the argument is optional
 *	  (if it isn't present, optarg is set to NULL).
 *	  Used for 'set -o'.
 *	- ',' is like ':' in options, except the argument always immediately
 *	  follows the option character (optarg is set to the null string if
 *	  the option is missing).
 *	  Used for 'read -u2', 'print -u2' and fc -40.
 *	- '#' is like ':' in options, expect that the argument is optional
 *	  and must start with a digit. If the argument doesn't start with a
 *	  digit, it is assumed to be missing and normal option processing
 *	  continues (optarg is set to 0 if the option is missing).
 *	  Used for 'typeset -LZ4'.
 *	- accepts +c as well as -c IF the GF_PLUSOPT flag is present. If an
 *	  option starting with + is accepted, the GI_PLUS flag will be set
 *	  in go->info.
 */
int
ksh_getopt(const char **argv, Getopt *go, const char *optionsp)
{
	char c;
	const char *o;

	if (go->p == 0 || (c = argv[go->optind - 1][go->p]) == '\0') {
		const char *arg = argv[go->optind], flag = arg ? *arg : '\0';

		go->p = 1;
		if (flag == '-' && ksh_isdash(arg + 1)) {
			go->optind++;
			go->p = 0;
			go->info |= GI_MINUSMINUS;
			return (-1);
		}
		if (arg == NULL ||
		    ((flag != '-' ) &&
		    /* neither a - nor a + (if + allowed) */
		    (!(go->flags & GF_PLUSOPT) || flag != '+')) ||
		    (c = arg[1]) == '\0') {
			go->p = 0;
			return (-1);
		}
		go->optind++;
		go->info &= ~(GI_MINUS|GI_PLUS);
		go->info |= flag == '-' ? GI_MINUS : GI_PLUS;
	}
	go->p++;
	if (ctype(c, C_QUEST | C_COLON | C_HASH) || c == ';' || c == ',' ||
	    !(o = cstrchr(optionsp, c))) {
		if (optionsp[0] == ':') {
			go->buf[0] = c;
			go->optarg = go->buf;
		} else {
			ksh_getopt_opterr(c,
			    (go->flags & GF_NONAME) ? null : argv[0],
			    Tunknown_option);
			if (go->flags & GF_ERROR)
				bi_unwind(1);
		}
		return (ORD('?'));
	}
	/**
	 * : means argument must be present, may be part of option argument
	 *   or the next argument
	 * ; same as : but argument may be missing
	 * , means argument is part of option argument, and may be null.
	 */
	if (*++o == ':' || *o == ';') {
		if (argv[go->optind - 1][go->p])
			go->optarg = argv[go->optind - 1] + go->p;
		else if (argv[go->optind])
			go->optarg = argv[go->optind++];
		else if (*o == ';')
			go->optarg = NULL;
		else {
			if (optionsp[0] == ':') {
				go->buf[0] = c;
				go->optarg = go->buf;
				return (ORD(':'));
			}
			ksh_getopt_opterr(c,
			    (go->flags & GF_NONAME) ? null : argv[0],
			    Treq_arg);
			if (go->flags & GF_ERROR)
				bi_unwind(1);
			return (ORD('?'));
		}
		go->p = 0;
	} else if (*o == ',') {
		/* argument is attached to option character, even if null */
		go->optarg = argv[go->optind - 1] + go->p;
		go->p = 0;
	} else if (*o == '#') {
		/*
		 * argument is optional and may be attached or unattached
		 * but must start with a digit. optarg is set to 0 if the
		 * argument is missing.
		 */
		if (argv[go->optind - 1][go->p]) {
			if (ctype(argv[go->optind - 1][go->p], C_DIGIT)) {
				go->optarg = argv[go->optind - 1] + go->p;
				go->p = 0;
			} else
				go->optarg = NULL;
		} else {
			if (argv[go->optind] &&
			    ctype(argv[go->optind][0], C_DIGIT)) {
				go->optarg = argv[go->optind++];
				go->p = 0;
			} else
				go->optarg = NULL;
		}
	}
	return (ord(c));
}

void
ksh_getopt_opterr(int ch, const char *name, const char *msg)
{
	static char buf[3] = { '-', KSH_BEL, '\0' };

	buf[1] = ch;
	if (name == null || name == kshname)
		kwarnf(KWF_ERR(1) | KWF_PREFIX | KWF_FILELINE |
		    KWF_TWOMSG | KWF_NOERRNO, buf, msg);
	else
		kwarnf(KWF_ERR(1) | KWF_PREFIX | KWF_FILELINE |
		    KWF_THREEMSG | KWF_NOERRNO, name, buf, msg);
}

/*
 * print variable/alias value using necessary quotes
 * (POSIX says they should be suitable for re-entry...)
 * No trailing newline is printed.
 */
void
print_value_quoted(struct shf *shf, const char *s)
{
	unsigned char c;
	const unsigned char *p = (const unsigned char *)s;
	Wahr inquote = Ja;

	/* first, special-case empty strings (for re-entrancy) */
	if (!*s) {
		shf_putc('\'', shf);
		shf_putc('\'', shf);
		return;
	}

	/* non-empty; check whether any quotes are needed */
	if (UTFMODE) {
		/* C1 always escaped; multibyte makes this tricky */
		while ((c = *p++) != 0) {
			if (ctype(c, C_CNTRL)) {
				dollarqU(shf, (const unsigned char *)s);
				return;
			}
			/* anything out of ASCII present? */
			if (rtt2asc(c) > 0x7EU) {
				/* dollar-quote, but be prepared to redo */
				char *guess;
				struct shf to;

				shf_sopen(NULL, 0, SHF_WR | SHF_DYNAMIC, &to);
				c = dollarqU(&to, (const unsigned char *)s);
				guess = shf_sclose(&to);
				/* output guess if it was right */
				if (c > 1)
					shf_puts(guess, shf);
				afree(guess, ATEMP);
				if (c == 1)
					goto always_single;
				if (c == 0)
 noquoteneeded:
					shf_puts(s, shf);
				return;
			}
			if (ctype(c, C_QUOTE | C_SPC))
				inquote = Nee;
		}
		/* assert: c == 0; all chars in [20;7E] ASCII */
#ifndef MKSH_EBCDIC
	} else if (Flag(FASIS)) {
		while ((c = *p++), !ksh_asisctrl(c))
			if (ctype(c, C_QUOTE | C_SPC))
				inquote = Nee;
#endif
	} else {
		while ((c = *p++), !ksh_isctrl(c))
			if (ctype(c, C_QUOTE | C_SPC))
				inquote = Nee;
	}
	/* state: if c == 0, all chars printable, inquote shortcuts */

	if (c) {
		/* otherwise, escape control chars */
		dollarq8(shf, (const unsigned char *)s);
		return;
	}

	/* can we shortcut? */
	if (inquote)
		goto noquoteneeded;
	/* no */
 always_single:
	/* all chars printable, no control chars, quote nicely */
	inquote = Nee;
	p = (const unsigned char *)s;

	while ((c = *p++) != 0) {
		if (c == '\'') {
			if (inquote) {
				shf_scheck(3, shf);
				shf_putc('\'', shf);
				inquote = Nee;
			}
			shf_putc('\\', shf);
		} else if (!inquote) {
			shf_putc('\'', shf);
			inquote = Ja;
		}
		shf_putc(c, shf);
	}
	if (inquote)
		shf_putc('\'', shf);
}

#define dollarq_Uctrl(c)	(!ctype((c), C_PRINT))
#ifndef MKSH_SMALL
#define dollarq_isctrlU(c)	dollarq_Uctrl(c)
#else
#define dollarq_isctrlU(c)	(UTFMODE ? dollarq_Uctrl(c) : ksh_isctrl8(c))
#endif

/* escape with $'...' (!MKSH_SMALL: in UTFMODE) */
static unsigned int
dollarqU(struct shf *shf, const unsigned char *s)
{
	unsigned char c;
	unsigned int wc;
	size_t n;
	unsigned int rv = 0;

	shf_putc('$', shf);
	shf_putc('\'', shf);
	while ((c = *s) != 0) {
		if (
#ifdef MKSH_SMALL
		    UTFMODE &&
#endif
		    rtt2asc(c) >= 0xC2U && (n = utf_mbtowc(&wc,
		    (const char *)s)) != (size_t)-1) {
			/* valid UTF-8 multibyte character > 0x7F */
			if ((wc ^ 0x80U) < 0x20U) {
				/* C1 control character */
				shf_fprintf(shf, "\\u%04X", wc);
				rv = 2;
				s += n;
			} else {
				/*
				 * print as-is; we assume the tty DTRT for
				 * interlinear annotations, LTR/RTL mark,
				 * U+2028, U+2029, U+2066..U+206F, etc.
				 */
				shf_wr_sm(s, n, shf);
			}
			continue;
		}
		++s;
		/* single octet */
		rv |= ctype(c, C_QUOTE | C_SPC);
		switch (c) {
		/* see unbksl() in this file for comments */
		case KSH_BEL:
			c = 'a';
			if (0)
				/* FALLTHROUGH */
		case '\b':
			  c = 'b';
			if (0)
				/* FALLTHROUGH */
		case '\f':
			  c = 'f';
			if (0)
				/* FALLTHROUGH */
		case '\n':
			  c = 'n';
			if (0)
				/* FALLTHROUGH */
		case '\r':
			  c = 'r';
			if (0)
				/* FALLTHROUGH */
		case '\t':
			  c = 't';
			if (0)
				/* FALLTHROUGH */
		case KSH_VTAB:
			  c = 'v';
			if (0)
				/* FALLTHROUGH */
		case KSH_ESC:
			/* take E not e because \e is \ in *roff */
			  c = 'E';
			rv = 2;
			/* FALLTHROUGH */
		case '\\':
			shf_putc('\\', shf);

			if (0)
				/* FALLTHROUGH */
		default:
			  if (dollarq_isctrlU(c)) {
				rv = 2;
				/* FALLTHROUGH */
		case '\'':
				shf_fprintf(shf, "\\%03o", c);
				break;
			}

			shf_putc(c, shf);
			break;
		}
	}
	shf_putc('\'', shf);
	return (rv);
}

#ifndef MKSH_SMALL
/* escape with $'...' outside UTFMODE */
static void
dollarq8(struct shf *shf, const unsigned char *s)
{
	unsigned char c;

	shf_putc('$', shf);
	shf_putc('\'', shf);
	while ((c = *s++) != 0) {
		/* single octet */
		switch (c) {
		/* see unbksl() in this file for comments */
		case KSH_BEL:
			c = 'a';
			if (0)
				/* FALLTHROUGH */
		case '\b':
			  c = 'b';
			if (0)
				/* FALLTHROUGH */
		case '\f':
			  c = 'f';
			if (0)
				/* FALLTHROUGH */
		case '\n':
			  c = 'n';
			if (0)
				/* FALLTHROUGH */
		case '\r':
			  c = 'r';
			if (0)
				/* FALLTHROUGH */
		case '\t':
			  c = 't';
			if (0)
				/* FALLTHROUGH */
		case KSH_VTAB:
			  c = 'v';
			if (0)
				/* FALLTHROUGH */
		case KSH_ESC:
			/* take E not e because \e is \ in *roff */
			  c = 'E';
			/* FALLTHROUGH */
		case '\\':
			shf_putc('\\', shf);

			if (0)
				/* FALLTHROUGH */
		default:
			  if (ksh_isctrl8(c)) {
				/* FALLTHROUGH */
		case '\'':
				shf_fprintf(shf, "\\%03o", c);
				break;
			}

			shf_putc(c, shf);
			break;
		}
	}
	shf_putc('\'', shf);
}
#endif

/*
 * Print things in columns and rows - func() is called to format
 * the i-th element
 */
void
print_columns(struct columnise_opts *opts, unsigned int n,
    void (*func)(char *, size_t, unsigned int, const void *),
    const void *arg, size_t max_oct, size_t max_colz)
{
	unsigned int i, r = 0, c, rows, cols, nspace, max_col;
	char *str;

	if (!n)
		return;

	if (max_colz > 2147483646) {
#ifndef MKSH_SMALL
		kwarnf0(KWF_INTERNAL | KWF_WARNING | KWF_NOERRNO,
		    "print_columns called with %s=%zu >= INT_MAX",
		    "max_col", max_colz);
#endif
		return;
	}
	max_col = (unsigned int)max_colz;

	if (max_oct > 2147483646) {
#ifndef MKSH_SMALL
		kwarnf0(KWF_INTERNAL | KWF_WARNING | KWF_NOERRNO,
		    "print_columns called with %s=%zu >= INT_MAX",
		    "max_oct", max_oct);
#endif
		return;
	}
	++max_oct;
	str = alloc(max_oct, ATEMP);

	/*
	 * We use (max_col + 2) to consider the separator space.
	 * Note that no spaces are printed after the last column
	 * to avoid problems with terminals that have auto-wrap,
	 * but we need to also take this into account in x_cols.
	 */
	cols = (x_cols + 1) / (max_col + 2);

	/* if we can only print one column anyway, skip the goo */
	if (cols < 2) {
		goto prcols_easy;
		while (r < n) {
			shf_putc(opts->linesep, opts->shf);
 prcols_easy:
			(*func)(str, max_oct, r++, arg);
			shf_puts(str, opts->shf);
		}
		goto out;
	}

	rows = (n + cols - 1) / cols;
	if (opts->prefcol && cols > rows) {
		cols = rows;
		rows = (n + cols - 1) / cols;
	}

	nspace = (x_cols - max_col * cols) / cols;
	if (nspace < 2)
		nspace = 2;
	max_col = -max_col;
	goto prcols_hard;
	while (r < rows) {
		shf_putc(opts->linesep, opts->shf);
 prcols_hard:
		for (c = 0; c < cols; c++) {
			if ((i = c * rows + r) >= n)
				break;
			(*func)(str, max_oct, i, arg);
			if (i + rows >= n)
				shf_puts(str, opts->shf);
			else
				shf_fprintf(opts->shf, "%*s%*s",
				    (int)max_col, str, (int)nspace, null);
		}
		++r;
	}
 out:
	if (opts->do_last)
		shf_putc(opts->linesep, opts->shf);
	afree(str, ATEMP);
}

/* strip all NUL bytes from buf; output is NUL-terminated if stripped */
void
strip_nuls(char *buf, size_t len)
{
	char *cp, *dp, *ep;

	rndpush(buf, len);
	if (!len || !(dp = memchr(buf, '\0', len)))
		return;

	ep = buf + len;
	cp = dp;

 cp_has_nul_byte:
	while (cp++ < ep && *cp == '\0')
		;	/* nothing */
	while (cp < ep && *cp != '\0')
		*dp++ = *cp++;
	if (cp < ep)
		goto cp_has_nul_byte;

	*dp = '\0';
}

/*
 * Like read(2), but if read fails due to non-blocking flag,
 * resets flag and restarts read.
 */
ssize_t
blocking_read(int fd, char *buf, size_t nbytes)
{
	ssize_t ret;
	Wahr tried_reset = Nee;

	while ((ret = read(fd, buf, nbytes)) < 0) {
		if (!tried_reset && errno == EAGAIN) {
			if (reset_nonblock(fd) > 0) {
				tried_reset = Ja;
				continue;
			}
			errno = EAGAIN;
		}
		break;
	}
	return (ret);
}

/*
 * Reset the non-blocking flag on the specified file descriptor.
 * Returns -1 if there was an error, 0 if non-blocking wasn't set,
 * 1 if it was.
 */
int
reset_nonblock(int fd)
{
	int flags;

	if ((flags = fcntl(fd, F_GETFL, 0)) < 0)
		return (-1);
	if (!(flags & O_NONBLOCK))
		return (0);
	flags &= ~O_NONBLOCK;
	if (fcntl(fd, F_SETFL, flags) < 0)
		return (-1);
	return (1);
}

/* getcwd(3) equivalent, allocates from ATEMP but doesn't resize */
char *
ksh_get_wd(void)
{
#ifdef MKSH__NO_PATH_MAX
	char *rv, *cp;

	if ((cp = get_current_dir_name())) {
		if (mksh_abspath(cp))
			strdupx(rv, cp, ATEMP);
		else
			rv = NULL;
		free_gnu_gcdn(cp);
	} else
		rv = NULL;
#else
	char *rv;

	if (!getcwd((rv = alloc(PATH_MAX + 1, ATEMP)), PATH_MAX) ||
	    !mksh_abspath(rv)) {
		afree(rv, ATEMP);
		rv = NULL;
	}
#endif

	return (rv);
}

#ifndef ELOOP
#define ELOOP		E2BIG
#endif

char *
do_realpath(const char *upath)
{
	char *xp, *ip, *tp, *ipath, *ldest = NULL;
	XString xs;
	size_t pos, len;
	ssize_t llen;
	struct stat sb;
#ifdef MKSH__NO_PATH_MAX
	off_t ldestlen = 0;
#define pathlen ((size_t)sb.st_size)
#define pathcnd ((ldestlen < 1) || ((ldestlen - 1) < sb.st_size))
#define ldestsz ((size_t)ldestlen)
#else
#define pathlen ((size_t)PATH_MAX)
#define pathcnd (!ldest)
#define ldestsz (pathlen + 1U)
#endif
	/* max. recursion depth */
	int symlinks = 32;

	if (mksh_abspath(upath)) {
		/* upath is an absolute pathname */
		strdupx(ipath, upath, ATEMP);
#ifdef MKSH_DOSPATH
	} else if (mksh_drvltr(upath)) {
		/* upath is a drive-relative pathname */
		if (getdrvwd(&ldest, ord(*upath)))
			return (NULL);
		/* A:foo -> A:/cwd/foo; A: -> A:/cwd */
		strpathx(ipath, ldest, upath + 2, 0);
#endif
	} else {
		/* upath is a relative pathname, prepend cwd */
		if ((tp = ksh_get_wd()) == NULL)
			return (NULL);
		strpathx(ipath, tp, upath, 1);
		afree(tp, ATEMP);
	}

	/* ipath and upath are in memory at the same time -> unchecked */
	Xinit(xs, xp, strlen(ip = ipath) + 1, ATEMP);

	/* now jump into the deep of the loop */
	goto beginning_of_a_pathname;

	while (*ip) {
		/* skip slashes in input */
		while (mksh_cdirsep(*ip))
			++ip;
		if (!*ip)
			break;

		/* get next pathname component from input */
		tp = ip;
		while (*ip && !mksh_cdirsep(*ip))
			++ip;
		len = ip - tp;

		/* check input for "." and ".." */
		if (tp[0] == '.') {
			if (len == 1)
				/* just continue with the next one */
				continue;
			else if (len == 2 && tp[1] == '.') {
				/* strip off last pathname component */
				/*XXX consider a rooted pathname */
				while (xp > Xstring(xs, xp))
					if (mksh_cdirsep(*--xp))
						break;
				/* then continue with the next one */
				continue;
			}
		}

		/* store output position away, then append slash to output */
		pos = Xsavepos(xs, xp);
		/* 1 for the '/' and len + 1 for tp and the NUL from below */
		XcheckN(xs, xp, 1 + len + 1);
		Xput(xs, xp, '/');

		/* append next pathname component to output */
		memcpy(xp, tp, len);
		xp += len;
		*xp = '\0';

		/* lstat the current output, see if it's a symlink */
		if (mksh_lstat(Xstring(xs, xp), &sb)) {
			/* lstat failed */
			if (errno == ENOENT) {
				/* because the pathname does not exist */
				while (mksh_cdirsep(*ip))
					/* skip any trailing slashes */
					++ip;
				/* no more components left? */
				if (!*ip)
					/* we can still return successfully */
					break;
				/* more components left? fall through */
			}
			/* not ENOENT or not at the end of ipath */
			goto notfound;
		}

		/* check if we encountered a symlink? */
		if (S_ISLNK(sb.st_mode)) {
#ifndef MKSH__NO_SYMLINK
			/* reached maximum recursion depth? */
			if (!symlinks--) {
				/* yep, prevent infinite loops */
				errno = ELOOP;
				goto notfound;
			}

			/* get symlink(7) target */
			if (pathcnd) {
#ifdef MKSH__NO_PATH_MAX
				/* same as notoktoadd(pathlen, 1) but adapted */
				if ((uintmax_t)sb.st_size >= (uintmax_t)mksh_MAXSZ) {
					errno = ENAMETOOLONG;
					goto notfound;
				}
				ldestlen = sb.st_size + 1; /* <= mksh_MAXSZ */
#endif
				/* ldestsz == pathlen + 1 */
				ldest = aresize(ldest, ldestsz, ATEMP);
			}
			errno = ENAMETOOLONG; /* for > pathlen case */
			llen = readlink(Xstring(xs, xp), ldest, ldestsz);
			if (llen < 0 || (size_t)llen > pathlen)
				/* oops... */
				goto notfound;
			ldest[llen] = '\0';

			/*
			 * restart if symlink target is an absolute path,
			 * otherwise continue with currently resolved prefix
			 */
#ifdef MKSH_DOSPATH
 assemble_symlink:
#endif
			/* append rest of current input path to link target */
			strpathx(tp, ldest, ip, 0);
			afree(ipath, ATEMP);
			ip = ipath = tp;
			if (!mksh_abspath(ipath)) {
#ifdef MKSH_DOSPATH
				/* symlink target might be drive-relative */
				if (mksh_drvltr(ipath)) {
					if (getdrvwd(&ldest, ord(*ipath)))
						goto notfound;
					ip += 2;
					goto assemble_symlink;
				}
#endif
				/* symlink target is a relative path */
				xp = Xrestpos(xs, xp, pos);
			} else
#endif
			  {
				/* symlink target is an absolute path */
				xp = Xstring(xs, xp);
 beginning_of_a_pathname:
				/* assert: mksh_abspath(ip == ipath) */
				/* assert: xp == xs.beg => start of path */

				/* exactly two leading slashes? (SUSv4 3.266) */
				if (ip[1] == ip[0] && !mksh_cdirsep(ip[2])) {
					/* keep them, e.g. for UNC pathnames */
					Xput(xs, xp, '/');
				}
#ifdef MKSH_DOSPATH
				/* drive letter? */
				if (mksh_drvltr(ip)) {
					/* keep it */
					Xput(xs, xp, *ip++);
					Xput(xs, xp, *ip++);
				}
#endif
			}
		}
		/* otherwise (no symlink) merely go on */
	}

	/*
	 * either found the target and successfully resolved it,
	 * or found its parent directory and may create it
	 */
	if (Xlength(xs, xp) == 0)
		/*
		 * if the resolved pathname is "", make it "/",
		 * otherwise do not add a trailing slash
		 */
		Xput(xs, xp, '/');
	Xput(xs, xp, '\0');

	/*
	 * if source path had a trailing slash, check if target path
	 * is not a non-directory existing file
	 */
	if (ip > ipath && mksh_cdirsep(ip[-1])) {
		if (stat(Xstring(xs, xp), &sb)) {
			if (errno != ENOENT)
				goto notfound;
		} else if (!S_ISDIR(sb.st_mode)) {
			errno = ENOTDIR;
			goto notfound;
		}
		/* target now either does not exist or is a directory */
	}

	/* return target path */
	afree(ldest, ATEMP);
	afree(ipath, ATEMP);
	return (Xclose(xs, xp));

 notfound:
	/* save; freeing memory might trash it */
	symlinks = errno;
	afree(ldest, ATEMP);
	afree(ipath, ATEMP);
	Xfree(xs, xp);
	errno = symlinks;
	return (NULL);

#undef pathlen
#undef pathcnd
}

/**
 *	Makes a filename into result using the following algorithm.
 *	- make result NULL
 *	- if file starts with '/', append file to result & set cdpathp to NULL
 *	- if file starts with ./ or ../ append cwd and file to result
 *	  and set cdpathp to NULL
 *	- if the first element of cdpathp doesn't start with a '/' xx or '.' xx
 *	  then cwd is appended to result.
 *	- the first element of cdpathp is appended to result
 *	- file is appended to result
 *	- cdpathp is set to the start of the next element in cdpathp (or NULL
 *	  if there are no more elements.
 *	The return value indicates whether a non-null element from cdpathp
 *	was appended to result.
 */
static int
make_path(const char *cwd, const char *file,
    /* pointer to colon-separated list */
    char **cdpathp,
    XString *xsp,
    int *phys_pathp)
{
	int rval = 0;
	Wahr use_cdpath = Ja;
	char *plist;
	size_t len, plen = 0;
	char *xp = Xstring(*xsp, xp);

	if (!file) {
		file = null;
#ifdef DEBUG
		assert(file[0] == '\0'); /* for Coverity */
#endif
	}

	if (mksh_abspath(file)) {
		*phys_pathp = 0;
		use_cdpath = Nee;
	} else {
		if (file[0] == '.') {
			char c = file[1];

			if (c == '.')
				c = file[2];
			if (mksh_cdirsep(c) || c == '\0')
				use_cdpath = Nee;
		}

		plist = *cdpathp;
		if (!plist)
			use_cdpath = Nee;
		else if (use_cdpath) {
			char *pend = plist;

			while (*pend && *pend != MKSH_PATHSEPC)
				++pend;
			plen = pend - plist;
			*cdpathp = *pend ? pend + 1 : NULL;
		}

		if ((!use_cdpath || !plen || !mksh_abspath(plist)) &&
		    (cwd && *cwd)) {
			len = strlen(cwd);
			XcheckN(*xsp, xp, len);
			memcpy(xp, cwd, len);
			xp += len;
			if (mksh_cdirsep(xp[-1]))
				xp--;
			*xp++ = '/';
		}
		*phys_pathp = Xlength(*xsp, xp);
		if (use_cdpath && plen) {
			XcheckN(*xsp, xp, plen);
			memcpy(xp, plist, plen);
			xp += plen;
			if (mksh_cdirsep(xp[-1]))
				xp--;
			*xp++ = '/';
			rval = 1;
		}
	}

	len = strlen(file) + 1;
	XcheckN(*xsp, xp, len);
	memcpy(xp, file, len);

	if (!use_cdpath)
		*cdpathp = NULL;

	return (rval);
}

/*-
 * Simplify pathnames containing "." and ".." entries.
 *
 * simplify_path(this)			= that
 * /a/b/c/./../d/..			/a/b
 * //./C/foo/bar/../baz			//C/foo/baz
 * /foo/				/foo
 * /foo/../../bar			/bar
 * /foo/./blah/..			/foo
 * .					.
 * ..					..
 * ./foo				foo
 * foo/../../../bar			../../bar
 * C:/foo/../..				C:/
 * C:.					C:
 * C:..					C:..
 * C:foo/../../blah			C:../blah
 *
 * XXX consider a rooted pathname: we cannot really 'cd ..' for
 * pathnames like: '/', 'c:/', '//foo', '//foo/', '/@unixroot/'
 * (no effect), 'c:', 'c:.' (effect is retaining the '../') but
 * we need to honour this throughout the shell
 */
void
simplify_path(char *p)
{
	char *dp, *ip, *sp, *tp;
	size_t len;
	Wahr needslash;
#ifdef MKSH_DOSPATH
	Wahr needdot = Ja;

	/* keep drive letter */
	if (mksh_drvltr(p)) {
		p += 2;
		needdot = Nee;
	}
#else
#define needdot Ja
#endif

	switch (*p) {
	case 0:
		return;
	case '/':
#ifdef MKSH_DOSPATH
	case '\\':
#endif
		/* exactly two leading slashes? (SUSv4 3.266) */
		if (p[1] == p[0] && !mksh_cdirsep(p[2])) {
			/* keep them, e.g. for UNC pathnames */
#ifdef MKSH_DOSPATH
			*p++ = '/';
#else
			++p;
#endif
		}
		needslash = Ja;
		break;
	default:
		needslash = Nee;
	}
	dp = ip = sp = p;

	while (*ip) {
		/* skip slashes in input */
		while (mksh_cdirsep(*ip))
			++ip;
		if (!*ip)
			break;

		/* get next pathname component from input */
		tp = ip;
		while (*ip && !mksh_cdirsep(*ip))
			++ip;
		len = ip - tp;

		/* check input for "." and ".." */
		if (tp[0] == '.') {
			if (len == 1)
				/* just continue with the next one */
				continue;
			else if (len == 2 && tp[1] == '.') {
				/* parent level, but how? (see above) */
				if (mksh_abspath(p))
					/* absolute path, only one way */
					goto strip_last_component;
				else if (dp > sp) {
					/* relative path, with subpaths */
					needslash = Nee;
 strip_last_component:
					/* strip off last pathname component */
					while (dp > sp)
						if (mksh_cdirsep(*--dp))
							break;
				} else {
					/* relative path, at its beginning */
					if (needslash)
						/* or already dotdot-slash'd */
						*dp++ = '/';
					/* keep dotdot-slash if not absolute */
					*dp++ = '.';
					*dp++ = '.';
					needslash = Ja;
					sp = dp;
				}
				/* then continue with the next one */
				continue;
			}
		}

		if (needslash)
			*dp++ = '/';

		/* append next pathname component to output */
		memmove(dp, tp, len);
		dp += len;

		/* append slash if we continue */
		needslash = Ja;
		/* try next component */
	}
	if (dp == p) {
		/* empty path -> dot (or slash, when absolute) */
		if (needslash)
			*dp++ = '/';
		else if (needdot)
			*dp++ = '.';
	}
	*dp = '\0';
#undef needdot
}

void
set_current_wd(const char *nwd)
{
	char *allocd = NULL;

	if (nwd == NULL) {
		allocd = ksh_get_wd();
		nwd = allocd ? allocd : null;
	}

	afree(current_wd, APERM);
	strdupx(current_wd, nwd, APERM);

	afree(allocd, ATEMP);
}

int
c_cd(const char **wp)
{
	int optc, rv, phys_path;
	Wahr physical = isWahr(Flag(FPHYSICAL));
	/* was a node from cdpath added in? */
	int cdnode;
	/* show where we went?, error for $PWD */
	Wahr printpath = Nee, eflag = Nee;
	struct tbl *pwd_s, *oldpwd_s;
	XString xs;
	char *dir, *allocd = NULL, *tryp, *pwd, *cdpath;

	while ((optc = ksh_getopt(wp, &builtin_opt, "eLP")) != -1)
		switch (optc) {
		case 'e':
			eflag = Ja;
			break;
		case 'L':
			physical = Nee;
			break;
		case 'P':
			physical = Ja;
			break;
		case '?':
			return (2);
		}
	wp += builtin_opt.optind;

	if (Flag(FRESTRICTED)) {
		bi_errorf(Tcant_cd);
		return (2);
	}

	pwd_s = global(TPWD);
	oldpwd_s = global(TOLDPWD);

	if (!wp[0]) {
		/* no arguments; go home */
		if ((dir = str_val(global("HOME"))) == null) {
			bi_errorf("no home directory (HOME not set)");
			return (2);
		}
	} else if (!wp[1]) {
		/* one argument: - or dir */
		if (ksh_isdash(wp[0])) {
			dir = str_val(oldpwd_s);
			if (dir == null) {
				bi_errorf(Tno_OLDPWD);
				return (2);
			}
			printpath = Ja;
		} else {
			strdupx(allocd, wp[0], ATEMP);
			dir = allocd;
		}
	} else if (!wp[2]) {
		/* two arguments; substitute arg1 in PWD for arg2 */
		size_t ilen, olen, nlen, elen;
		char *cp;

		if (!current_wd[0]) {
			bi_errorf("can't determine current directory");
			return (2);
		}
		/*
		 * Substitute arg1 for arg2 in current path. If the first
		 * substitution fails because the cd fails we could try to
		 * find another substitution. For now, we don't.
		 */
		if ((cp = ucstrstr(current_wd, wp[0])) == NULL) {
			bi_errorf(Tbadsubst);
			return (2);
		}
		/*-
		 * ilen = part of current_wd before wp[0]
		 * elen = part of current_wd after wp[0]
		 * because current_wd and wp[1] need to be in memory at the
		 * same time beforehand the addition can stay unchecked
		 */
		ilen = cp - current_wd;
		olen = strlen(wp[0]);
		nlen = strlen(wp[1]);
		elen = strlen(current_wd + ilen + olen) + 1;
		dir = allocd = alloc(ilen + nlen + elen, ATEMP);
		memcpy(dir, current_wd, ilen);
		memcpy(dir + ilen, wp[1], nlen);
		memcpy(dir + ilen + nlen, current_wd + ilen + olen, elen);
		printpath = Ja;
	} else {
		bi_errorf(Ttoo_many_args);
		return (2);
	}

#ifdef MKSH_DOSPATH
	tryp = NULL;
	if (mksh_drvltr(dir) && !mksh_cdirsep(dir[2]) &&
	    !getdrvwd(&tryp, ord(*dir))) {
		strpathx(dir, tryp, dir + 2, 0);
		afree(tryp, ATEMP);
		afree(allocd, ATEMP);
		allocd = dir;
	}
#endif

#ifdef MKSH__NO_PATH_MAX
	/* only a first guess; make_path will enlarge xs if necessary */
	XinitN(xs, 1024, ATEMP);
#else
	XinitN(xs, PATH_MAX, ATEMP);
#endif

	cdpath = str_val(global("CDPATH"));
	do {
		cdnode = make_path(current_wd, dir, &cdpath, &xs, &phys_path);
		if (physical)
			rv = chdir(tryp = Xstring(xs, xp) + phys_path);
		else {
			simplify_path(Xstring(xs, xp));
			rv = chdir(tryp = Xstring(xs, xp));
		}
	} while (rv < 0 && cdpath != NULL);

	if (rv < 0) {
		if (cdnode)
			bi_errorf(Tf_sD_s, dir, "bad directory");
		else
			bi_errorf(Tf_sD_s, tryp, cstrerror(errno));
		afree(allocd, ATEMP);
		Xfree(xs, xp);
		return (2);
	}

	rv = 0;

	/* allocd (above) => dir, which is no longer used */
	afree(allocd, ATEMP);
	allocd = NULL;

	/* Clear out tracked aliases with relative paths */
	flushcom(Nee);

	/*
	 * Set OLDPWD (note: unsetting OLDPWD does not disable this
	 * setting in AT&T ksh)
	 */
	if (current_wd[0])
		/* Ignore failure (happens if read-only or integer) */
		setstr(oldpwd_s, current_wd, KSH_RETURN_ERROR);

	if (!mksh_abspath(Xstring(xs, xp))) {
		pwd = NULL;
	} else if (!physical) {
		goto norealpath_PWD;
	} else if ((pwd = allocd = do_realpath(Xstring(xs, xp))) == NULL) {
		if (eflag)
			rv = 1;
 norealpath_PWD:
		pwd = Xstring(xs, xp);
	}

	/* Set PWD */
	if (pwd) {
		char *ptmp = pwd;

		set_current_wd(ptmp);
		/* Ignore failure (happens if read-only or integer) */
		setstr(pwd_s, ptmp, KSH_RETURN_ERROR);
	} else {
		set_current_wd(null);
		pwd = Xstring(xs, xp);
		/* XXX unset $PWD? */
		if (eflag)
			rv = 1;
	}
	if (printpath || cdnode)
		shprintf(Tf_sN, pwd);

	afree(allocd, ATEMP);
	Xfree(xs, xp);
	return (rv);
}

extern void chvt_reinit(void);

static void
chvt(const Getopt *go)
{
	char buf[99], ch;
	const char *dv = go->optarg;
	int fd, pfd[2];
	pid_t cpid;
	Wahr isdaemon = Nee, dowait = Nee;
#ifndef MKSH_DISABLE_REVOKE_WARNING
	int revwarn = 0;
#if !HAVE_REVOKE
#define ifrevwarn
#else
#define ifrevwarn if (revwarn)
#endif
#endif

	switch (ord(*dv)) {
	case ORD('-'):
		isdaemon = Ja;
		dv = "/dev/null";
		break;
	case ORD('!'):
		dowait = Ja;
		++dv;
		/* FALLTHROUGH */
	default: {
		struct stat sb;

		if (stat(dv, &sb)) {
			int E = errno;

			memcpy(buf, "/dev/ttyC", 9U);
			strlcpy(buf + 9U, dv, sizeof(buf) - 9U);
			if (stat(buf, &sb)) {
				strlcpy(buf + 8U, dv, sizeof(buf) - 8U);
				if (stat(buf, &sb))
					kerrf(KWF_VERRNO | KWF_ERR(1) |
					    KWF_PREFIX | KWF_TWOMSG, E,
					    "chvt", dv);
			}
			dv = buf;
		}
		if (!S_ISCHR(sb.st_mode))
			kerrf(KWF_VERRNO | KWF_ERR(1) | KWF_PREFIX |
			    KWF_TWOMSG, (int)(ENOTTY), "chvt", dv);
#ifndef MKSH_DISABLE_REVOKE_WARNING
#if !HAVE_REVOKE
#ifdef ENOSYS
		revwarn = ENOSYS;
#else
		revwarn = EINVAL;
#endif
#else
		revwarn = revoke(dv) ? errno : 0;
#endif
		ifrevwarn kwarnf(KWF_VERRNO | KWF_PREFIX | KWF_THREEMSG,
		    revwarn, "chvt", dv,
		    "can't revoke; new shell is potentially insecure");
#endif
	    }
	}
	if (pipe(pfd))
		kerrf(KWF_ERR(1) | KWF_PREFIX | KWF_TWOMSG, "chvt", "pipe");
	switch ((cpid = fork())) {
	case -1:
		close(pfd[0]);
		close(pfd[1]);
		kerrf(KWF_ERR(1) | KWF_PREFIX | KWF_TWOMSG, "chvt", "fork");
	case 0:
		close(pfd[0]);
		break;
	default:
		close(pfd[1]);
		if (read(pfd[0], &ch, 1) != 1 || !isch(ch, '.'))
			kerrf(KWF_ERR(1) | KWF_PREFIX | KWF_THREEMSG |
			    KWF_NOERRNO, "chvt", dv,
			    "child process initialisation failure");
		close(pfd[0]);
		if (dowait) {
			pid_t corpse;
			int status;

			while (/* CONSTCOND */ 1) {
#ifndef MKSH_NOPROSPECTOFWORK
				corpse = waitpid(cpid, &status, 0);
#else
				corpse = wait(&status);
#endif
				if (corpse == -1) {
					status = errno;
					if (status == ECHILD)
						break;
					if (status == EINTR)
						continue;
					kerrf(KWF_ERR(1) | KWF_PREFIX |
					    KWF_TWOMSG, "chvt", "wait");
				}
#ifdef MKSH_NOPROSPECTOFWORK
				/* should not happen but… */
				if (corpse != cpid)
					continue;
#endif
				if (WIFSIGNALED(status)) {
					status = WTERMSIG(status);
					dv = status > 0 && status < ksh_NSIG ?
					    ksh_sigmess(status) : NULL;
					if (ksh_sigmessf(dv))
						dv = "Signalled";
					kwarnf(KWF_PREFIX | KWF_TWOMSG |
					    KWF_NOERRNO, "chvt", dv);
				} else if (WIFEXITED(status)) {
					status = (WEXITSTATUS(status)) & 255;
					if (!status)
						break;
					kwarnf0(KWF_PREFIX | KWF_NOERRNO,
					    TchvtDone, status);
				} else
					continue;
			}
		}
		exit(0);
	}
	if (setsid() == -1)
		kerrf(KWF_ERR(1) | KWF_PREFIX | KWF_TWOMSG, "chvt", "setsid");
	if ((fd = binopen2(dv, O_RDWR)) < 0) {
		sleep(1);
		if ((fd = binopen2(dv, O_RDWR)) < 0)
			kerrf(KWF_ERR(1) | KWF_PREFIX | KWF_THREEMSG,
			    "chvt", dv, Topen);
	}
	if (!isdaemon) {
#ifdef TIOCSCTTY
		if (ioctl(fd, TIOCSCTTY, NULL) == -1)
			kerrf(KWF_ERR(1) | KWF_PREFIX | KWF_TWOMSG,
			    "chvt", "TIOCSCTTY");
#endif
#if HAVE_TERMIOS_H
		if (tcflush(fd, TCIOFLUSH))
			kerrf(KWF_ERR(1) | KWF_PREFIX | KWF_TWOMSG,
			    "chvt", "TCIOFLUSH");
#else
		if (ioctl(fd, TCFLSH, 2) == -1)
			kerrf(KWF_ERR(1) | KWF_PREFIX | KWF_TWOMSG,
			    "chvt", "TCFLSH");
#endif
	}
	ksh_dup2(fd, 0, Nee);
	ksh_dup2(fd, 1, Nee);
	ksh_dup2(fd, 2, Nee);
#ifndef MKSH_DISABLE_REVOKE_WARNING
	if (!isdaemon) {
		ifrevwarn kwarnf(KWF_VERRNO | KWF_PREFIX | KWF_THREEMSG,
		    revwarn, "chvt", dv,
		    "can't revoke; new shell is potentially insecure");
	}
#endif
	if (fd > 2)
		close(fd);
	rndset((unsigned long)chvt_rndsetup(go, sizeof(Getopt)));
	chvt_reinit();
	/* signal parent the all OK */
	if (write(pfd[1], Tdot, 1) != 1)
		kwarnf(KWF_PREFIX | KWF_TWOMSG, "chvt", "write");
	close(pfd[1]);
}

#if defined(MKSH_SMALL) && !defined(MKSH_SMALL_BUT_FAST)
char *
strndup_i(const char *src, size_t len, Area *ap)
{
	char *dst = NULL;

	if (src != NULL) {
		dst = alloc(len + 1, ap);
		memcpy(dst, src, len);
		dst[len] = '\0';
	}
	return (dst);
}

char *
strdup_i(const char *src, Area *ap)
{
	return (src == NULL ? NULL : strndup_i(src, strlen(src), ap));
}
#endif

#if !HAVE_GETRUSAGE
#define INVTCK(r,t)	do {						\
	r.tv_usec = ((t) % (1000000 / CLK_TCK)) * (1000000 / CLK_TCK);	\
	r.tv_sec = (t) / CLK_TCK;					\
} while (/* CONSTCOND */ 0)

int
ksh_getrusage(int what, struct rusage *ru)
{
	struct tms tms;
	clock_t u, s;
#ifndef CLK_TCK
	long CLK_TCK;
#endif

	if (/* ru == NULL || */ times(&tms) == (clock_t)-1)
		return (-1);

	switch (what) {
	case RUSAGE_SELF:
		u = tms.tms_utime;
		s = tms.tms_stime;
		break;
	case RUSAGE_CHILDREN:
		u = tms.tms_cutime;
		s = tms.tms_cstime;
		break;
	default:
		errno = EINVAL;
		return (-1);
	}
#ifndef CLK_TCK
#ifdef ENOSYS
	errno = ENOSYS;
#else
	errno = EINVAL;
#endif
	if ((CLK_TCK = sysconf(_SC_CLK_TCK)) == -1L)
		kerrf(KWF_INTERNAL | KWF_ERR(0xFF) | KWF_ONEMSG,
		    "sysconf(_SC_CLK_TCK)");
#endif
	INVTCK(ru->ru_utime, u);
	INVTCK(ru->ru_stime, s);
	return (0);
}
#endif

/*
 * process the string available via fg (get a char)
 * and fp (put back a char) for backslash escapes,
 * assuming the first call to *fg gets the char di-
 * rectly after the backslash; return the character
 * (0..0xFF), UCS (wc + 0x100), or -1 if no known
 * escape sequence was found
 */
int
unbksl(Wahr cstyle, int (*fg)(void), void (*fp)(int))
{
	int wc, i, c, fc, n;

	fc = (*fg)();
	switch (fc) {
	case 'a':
		wc = KSH_BEL;
		break;
	case 'b':
		wc = '\b';
		break;
	case 'c':
		if (!cstyle)
			goto unknown_escape;
		c = (*fg)();
		wc = asc2rtt(ord(c) == ORD('?') ? 0x7F : rtt2asc(c) & 0x9F);
		break;
	case 'E':
	case 'e':
		wc = KSH_ESC;
		break;
	case 'f':
		wc = '\f';
		break;
	case 'n':
		wc = '\n';
		break;
	case 'r':
		wc = '\r';
		break;
	case 't':
		wc = '\t';
		break;
	case 'v':
		wc = KSH_VTAB;
		break;
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
		if (!cstyle)
			goto unknown_escape;
		/* FALLTHROUGH */
	case '0':
		if (cstyle)
			(*fp)(fc);
		/*
		 * look for an octal number with up to three
		 * digits, not counting the leading zero;
		 * convert it to a raw octet
		 */
		wc = 0;
		i = 3;
		while (i--)
			if (ctype((c = (*fg)()), C_OCTAL))
				wc = (wc << 3) + ksh_numdig(c);
			else {
				(*fp)(c);
				break;
			}
		break;
	case 'U':
		i = 8;
		if (/* CONSTCOND */ 0)
			/* FALLTHROUGH */
	case 'u':
		  i = 4;
		if (/* CONSTCOND */ 0)
			/* FALLTHROUGH */
	case 'x':
		  i = cstyle ? -1 : 2;
		/**
		 * x:	look for a hexadecimal number with up to
		 *	two (C style: arbitrary) digits; convert
		 *	to raw octet (C style: UCS if >0xFF)
		 * u/U:	look for a hexadecimal number with up to
		 *	four (U: eight) digits; convert to UCS
		 */
		wc = 0;
		n = 0;
		while (n < i || i == -1) {
			wc <<= 4;
			if (!ctype((c = (*fg)()), C_SEDEC)) {
				wc >>= 4;
				(*fp)(c);
				break;
			}
			if (ctype(c, C_DIGIT))
				wc += ksh_numdig(c);
			else if (ctype(c, C_UPPER))
				wc += ksh_numuc(c) + 10;
			else
				wc += ksh_numlc(c) + 10;
			++n;
		}
		if (!n)
			goto unknown_escape;
		if ((cstyle && wc > 0xFF) || fc != 'x')
			/* UCS marker */
			wc += 0x100;
		break;
	case '\'':
		if (!cstyle)
			goto unknown_escape;
		wc = '\'';
		break;
	case '\\':
		wc = '\\';
		break;
	default:
 unknown_escape:
		(*fp)(fc);
		return (-1);
	}

	return (wc);
}

#ifdef DEBUG
#undef strchr
/* pre-initio() */
char *
ucstrchr(char *s, int c)
{
	return (strchr(s, c));
}

const char *
cstrchr(const char *s, int c)
{
	return (strchr(s, c));
}
#endif

#if !HAVE_STRSTR
char *
ucstrstr(char *big, const char *little)
{
	union mksh_cchack res;

	res.ro = cstrstr(big, little);
	return (res.rw);
}

const char *
cstrstr(const char *big, const char *little)
{
	char first, c;
	size_t n;

	if ((first = *little++) == '\0')
		return (big);
	n = strlen(little);
 strstr_look:
	while ((c = *big++) != first)
		if (c == '\0')
			return (NULL);
	if (strncmp(big, little, n))
		goto strstr_look;
	return (big - 1);
}
#elif defined(DEBUG)
#undef strstr
char *
ucstrstr(char *big, const char *little)
{
	return (strstr(big, little));
}

const char *
cstrstr(const char *big, const char *little)
{
	return (strstr(big, little));
}
#endif
