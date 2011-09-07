/*	$OpenBSD: c_ksh.c,v 1.33 2009/02/07 14:03:24 kili Exp $	*/
/*	$OpenBSD: c_sh.c,v 1.41 2010/03/27 09:10:01 jmc Exp $	*/
/*	$OpenBSD: c_test.c,v 1.18 2009/03/01 20:11:06 otto Exp $	*/
/*	$OpenBSD: c_ulimit.c,v 1.17 2008/03/21 12:51:19 millert Exp $	*/

/*-
 * Copyright (c) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009,
 *		 2010, 2011
 *	Thorsten Glaser <tg@mirbsd.org>
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

#if HAVE_SELECT
#if HAVE_SYS_BSDTYPES_H
#include <sys/bsdtypes.h>
#endif
#if HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#if HAVE_BSTRING_H
#include <bstring.h>
#endif
#endif

__RCSID("$MirOS: src/bin/mksh/funcs.c,v 1.197 2011/09/07 15:24:15 tg Exp $");

#if HAVE_KILLPG
/*
 * use killpg if < -1 since -1 does special things
 * for some non-killpg-endowed kills
 */
#define mksh_kill(p,s)	((p) < -1 ? killpg(-(p), (s)) : kill((p), (s)))
#else
/* cross fingers and hope kill is killpg-endowed */
#define mksh_kill	kill
#endif

/* XXX conditions correct? */
#if !defined(RLIM_INFINITY) && !defined(MKSH_NO_LIMITS)
#define MKSH_NO_LIMITS	1
#endif

#ifdef MKSH_NO_LIMITS
#define c_ulimit	c_true
#endif

#if defined(ANDROID)
static int c_android_lsmod(const char **);
#endif

static int
c_true(const char **wp MKSH_A_UNUSED)
{
	return (0);
}

static int
c_false(const char **wp MKSH_A_UNUSED)
{
	return (1);
}

/*
 * A leading = means assignments before command are kept;
 * a leading * means a POSIX special builtin;
 * a leading + means a POSIX regular builtin
 * (* and + should not be combined).
 */
const struct builtin mkshbuiltins[] = {
	{"*=.", c_dot},
	{"*=:", c_true},
	{"[", c_test},
	{"*=break", c_brkcont},
	{Tgbuiltin, c_builtin},
	{"*=continue", c_brkcont},
	{"*=eval", c_eval},
	{"*=exec", c_exec},
	{"*=exit", c_exitreturn},
	{"+false", c_false},
	{"*=return", c_exitreturn},
	{Tsgset, c_set},
	{"*=shift", c_shift},
	{"=times", c_times},
	{"*=trap", c_trap},
	{"+=wait", c_wait},
	{"+read", c_read},
	{"test", c_test},
	{"+true", c_true},
	{"ulimit", c_ulimit},
	{"+umask", c_umask},
	{"*=unset", c_unset},
	/* no =: AT&T manual wrong */
	{Tpalias, c_alias},
	{"+cd", c_cd},
	/* dash compatibility hack */
	{"chdir", c_cd},
	{"+command", c_command},
	{"echo", c_print},
	{"*=export", c_typeset},
	{"+fc", c_fc},
	{"+getopts", c_getopts},
	{"=global", c_typeset},
	{"+jobs", c_jobs},
	{"+kill", c_kill},
	{"let", c_let},
	{"print", c_print},
#ifdef MKSH_PRINTF_BUILTIN
	{"printf", c_printf},
#endif
	{"pwd", c_pwd},
	{"*=readonly", c_typeset},
	{T_typeset, c_typeset},
	{Tpunalias, c_unalias},
	{"whence", c_whence},
#ifndef MKSH_UNEMPLOYED
	{"+bg", c_fgbg},
	{"+fg", c_fgbg},
#endif
	{"bind", c_bind},
	{"cat", c_cat},
#if HAVE_MKNOD
	{"mknod", c_mknod},
#endif
	{"realpath", c_realpath},
	{"rename", c_rename},
#if HAVE_SELECT
	{"sleep", c_sleep},
#endif
#ifdef __MirBSD__
	/* alias to "true" for historical reasons */
	{"domainname", c_true},
#endif
#if defined(ANDROID)
	{"lsmod", c_android_lsmod},
#endif
	{NULL, (int (*)(const char **))NULL}
};

struct kill_info {
	int num_width;
	int name_width;
};

static const struct t_op {
	char op_text[4];
	Test_op op_num;
} u_ops[] = {
	{"-a",	TO_FILAXST },
	{"-b",	TO_FILBDEV },
	{"-c",	TO_FILCDEV },
	{"-d",	TO_FILID },
	{"-e",	TO_FILEXST },
	{"-f",	TO_FILREG },
	{"-G",	TO_FILGID },
	{"-g",	TO_FILSETG },
	{"-h",	TO_FILSYM },
	{"-H",	TO_FILCDF },
	{"-k",	TO_FILSTCK },
	{"-L",	TO_FILSYM },
	{"-n",	TO_STNZE },
	{"-O",	TO_FILUID },
	{"-o",	TO_OPTION },
	{"-p",	TO_FILFIFO },
	{"-r",	TO_FILRD },
	{"-s",	TO_FILGZ },
	{"-S",	TO_FILSOCK },
	{"-t",	TO_FILTT },
	{"-u",	TO_FILSETU },
	{"-w",	TO_FILWR },
	{"-x",	TO_FILEX },
	{"-z",	TO_STZER },
	{"",	TO_NONOP }
};
static const struct t_op b_ops[] = {
	{"=",	TO_STEQL },
	{"==",	TO_STEQL },
	{"!=",	TO_STNEQ },
	{"<",	TO_STLT },
	{">",	TO_STGT },
	{"-eq",	TO_INTEQ },
	{"-ne",	TO_INTNE },
	{"-gt",	TO_INTGT },
	{"-ge",	TO_INTGE },
	{"-lt",	TO_INTLT },
	{"-le",	TO_INTLE },
	{"-ef",	TO_FILEQ },
	{"-nt",	TO_FILNT },
	{"-ot",	TO_FILOT },
	{"",	TO_NONOP }
};

static int test_oexpr(Test_env *, bool);
static int test_aexpr(Test_env *, bool);
static int test_nexpr(Test_env *, bool);
static int test_primary(Test_env *, bool);
static Test_op ptest_isa(Test_env *, Test_meta);
static const char *ptest_getopnd(Test_env *, Test_op, bool);
static void ptest_error(Test_env *, int, const char *);
static char *kill_fmt_entry(char *, size_t, int, const void *);
static void p_time(struct shf *, bool, long, int, int,
    const char *, const char *)
    MKSH_A_NONNULL((__nonnull__ (6, 7)));

int
c_pwd(const char **wp)
{
	int optc;
	bool physical = tobool(Flag(FPHYSICAL));
	char *p, *allocd = NULL;

	while ((optc = ksh_getopt(wp, &builtin_opt, "LP")) != -1)
		switch (optc) {
		case 'L':
			physical = false;
			break;
		case 'P':
			physical = true;
			break;
		case '?':
			return (1);
		}
	wp += builtin_opt.optind;

	if (wp[0]) {
		bi_errorf("too many arguments");
		return (1);
	}
	p = current_wd[0] ? (physical ? allocd = do_realpath(current_wd) :
	    current_wd) : NULL;
	/* LINTED use of access */
	if (p && access(p, R_OK) < 0)
		p = NULL;
	if (!p && !(p = allocd = ksh_get_wd())) {
		bi_errorf("%s: %s", "can't determine current directory",
		    strerror(errno));
		return (1);
	}
	shprintf("%s\n", p);
	afree(allocd, ATEMP);
	return (0);
}

static const char *s_ptr;
static int s_get(void);
static void s_put(int);

int
c_print(const char **wp)
{
#define PO_NL		BIT(0)	/* print newline */
#define PO_EXPAND	BIT(1)	/* expand backslash sequences */
#define PO_PMINUSMINUS	BIT(2)	/* print a -- argument */
#define PO_HIST		BIT(3)	/* print to history instead of stdout */
#define PO_COPROC	BIT(4)	/* printing to coprocess: block SIGPIPE */
	int fd = 1, c;
	int flags = PO_EXPAND|PO_NL;
	const char *s, *emsg;
	XString xs;
	char *xp;

	if (wp[0][0] == 'e') {
		/* echo builtin */
		wp++;
		if (Flag(FPOSIX) || Flag(FSH) || Flag(FAS_BUILTIN)) {
			/* Debian Policy 10.4 compliant "echo" builtin */
			if (*wp && !strcmp(*wp, "-n")) {
				/* we recognise "-n" only as the first arg */
				flags = 0;
				wp++;
			} else
				/* otherwise, we print everything as-is */
				flags = PO_NL;
		} else {
			int nflags = flags;

			/**
			 * a compromise between sysV and BSD echo commands:
			 * escape sequences are enabled by default, and -n,
			 * -e and -E are recognised if they appear in argu-
			 * ments with no illegal options (ie, echo -nq will
			 * print -nq).
			 * Different from sysV echo since options are reco-
			 * gnised, different from BSD echo since escape se-
			 * quences are enabled by default.
			 */

			while ((s = *wp) && *s == '-' && s[1]) {
				while (*++s)
					if (*s == 'n')
						nflags &= ~PO_NL;
					else if (*s == 'e')
						nflags |= PO_EXPAND;
					else if (*s == 'E')
						nflags &= ~PO_EXPAND;
					else
						/*
						 * bad option: don't use
						 * nflags, print argument
						 */
						break;

				if (*s)
					break;
				wp++;
				flags = nflags;
			}
		}
	} else {
		int optc;
		const char *opts = "Rnprsu,";

		while ((optc = ksh_getopt(wp, &builtin_opt, opts)) != -1)
			switch (optc) {
			case 'R':
				/* fake BSD echo command */
				flags |= PO_PMINUSMINUS;
				flags &= ~PO_EXPAND;
				opts = "ne";
				break;
			case 'e':
				flags |= PO_EXPAND;
				break;
			case 'n':
				flags &= ~PO_NL;
				break;
			case 'p':
				if ((fd = coproc_getfd(W_OK, &emsg)) < 0) {
					bi_errorf("%s: %s", "-p", emsg);
					return (1);
				}
				break;
			case 'r':
				flags &= ~PO_EXPAND;
				break;
			case 's':
				flags |= PO_HIST;
				break;
			case 'u':
				if (!*(s = builtin_opt.optarg))
					fd = 0;
				else if ((fd = check_fd(s, W_OK, &emsg)) < 0) {
					bi_errorf("%s: %s: %s", "-u", s, emsg);
					return (1);
				}
				break;
			case '?':
				return (1);
			}

		if (!(builtin_opt.info & GI_MINUSMINUS)) {
			/* treat a lone - like -- */
			if (wp[builtin_opt.optind] &&
			    ksh_isdash(wp[builtin_opt.optind]))
				builtin_opt.optind++;
		} else if (flags & PO_PMINUSMINUS)
			builtin_opt.optind--;
		wp += builtin_opt.optind;
	}

	Xinit(xs, xp, 128, ATEMP);

	while (*wp != NULL) {
		s = *wp;
		while ((c = *s++) != '\0') {
			Xcheck(xs, xp);
			if ((flags & PO_EXPAND) && c == '\\') {
				s_ptr = s;
				c = unbksl(false, s_get, s_put);
				s = s_ptr;
				if (c == -1) {
					/* rejected by generic function */
					switch ((c = *s++)) {
					case 'c':
						flags &= ~PO_NL;
						/* AT&T brain damage */
						continue;
					case '\0':
						s--;
						c = '\\';
						break;
					default:
						Xput(xs, xp, '\\');
					}
				} else if ((unsigned int)c > 0xFF) {
					/* generic function returned Unicode */
					char ts[4];

					ts[utf_wctomb(ts, c - 0x100)] = 0;
					for (c = 0; ts[c]; ++c)
						Xput(xs, xp, ts[c]);
					continue;
				}
			}
			Xput(xs, xp, c);
		}
		if (*++wp != NULL)
			Xput(xs, xp, ' ');
	}
	if (flags & PO_NL)
		Xput(xs, xp, '\n');

	if (flags & PO_HIST) {
		Xput(xs, xp, '\0');
		histsave(&source->line, Xstring(xs, xp), true, false);
		Xfree(xs, xp);
	} else {
		int len = Xlength(xs, xp);
		int opipe = 0;

		/*
		 * Ensure we aren't killed by a SIGPIPE while writing to
		 * a coprocess. AT&T ksh doesn't seem to do this (seems
		 * to just check that the co-process is alive which is
		 * not enough).
		 */
		if (coproc.write >= 0 && coproc.write == fd) {
			flags |= PO_COPROC;
			opipe = block_pipe();
		}
		for (s = Xstring(xs, xp); len > 0; ) {
			if ((c = write(fd, s, len)) < 0) {
				if (flags & PO_COPROC)
					restore_pipe(opipe);
				if (errno == EINTR) {
					/* allow user to ^C out */
					intrcheck();
					if (flags & PO_COPROC)
						opipe = block_pipe();
					continue;
				}
				return (1);
			}
			s += c;
			len -= c;
		}
		if (flags & PO_COPROC)
			restore_pipe(opipe);
	}

	return (0);
}

static int
s_get(void)
{
	return (*s_ptr++);
}

static void
s_put(int c MKSH_A_UNUSED)
{
	--s_ptr;
}

int
c_whence(const char **wp)
{
	struct tbl *tp;
	const char *id;
	bool pflag = false, vflag = false, Vflag = false;
	int rv = 0, optc, fcflags;
	bool iam_whence = wp[0][0] == 'w';
	const char *opts = iam_whence ? "pv" : "pvV";

	while ((optc = ksh_getopt(wp, &builtin_opt, opts)) != -1)
		switch (optc) {
		case 'p':
			pflag = true;
			break;
		case 'v':
			vflag = true;
			break;
		case 'V':
			Vflag = true;
			break;
		case '?':
			return (1);
		}
	wp += builtin_opt.optind;

	fcflags = FC_BI | FC_PATH | FC_FUNC;
	if (!iam_whence) {
		/* Note that -p on its own is deal with in comexec() */
		if (pflag)
			fcflags |= FC_DEFPATH;
		/*
		 * Convert command options to whence options - note that
		 * command -pV uses a different path search than whence -v
		 * or whence -pv. This should be considered a feature.
		 */
		vflag = Vflag;
	}
	if (pflag)
		fcflags &= ~(FC_BI | FC_FUNC);

	while ((vflag || rv == 0) && (id = *wp++) != NULL) {
		uint32_t h = 0;

		tp = NULL;
		if ((iam_whence || vflag) && !pflag)
			tp = ktsearch(&keywords, id, h = hash(id));
		if (!tp && !pflag) {
			tp = ktsearch(&aliases, id, h ? h : hash(id));
			if (tp && !(tp->flag & ISSET))
				tp = NULL;
		}
		if (!tp)
			tp = findcom(id, fcflags);
		if (vflag || (tp->type != CALIAS && tp->type != CEXEC &&
		    tp->type != CTALIAS))
			shf_puts(id, shl_stdout);
		if (vflag)
			switch (tp->type) {
			case CKEYWD:
			case CALIAS:
			case CFUNC:
			case CSHELL:
				shf_puts(" is a", shl_stdout);
				break;
			}

		switch (tp->type) {
		case CKEYWD:
			if (vflag)
				shf_puts(" reserved word", shl_stdout);
			break;
		case CALIAS:
			if (vflag)
				shprintf("n %s%s for ",
				    (tp->flag & EXPORT) ? "exported " : null,
				    Talias);
			if (!iam_whence && !vflag)
				shprintf("%s %s=", Talias, id);
			print_value_quoted(tp->val.s);
			break;
		case CFUNC:
			if (vflag) {
				if (tp->flag & EXPORT)
					shf_puts("n exported", shl_stdout);
				if (tp->flag & TRACE)
					shf_puts(" traced", shl_stdout);
				if (!(tp->flag & ISSET)) {
					shf_puts(" undefined", shl_stdout);
					if (tp->u.fpath)
						shprintf(" (autoload from %s)",
						    tp->u.fpath);
				}
				shf_puts(T_function, shl_stdout);
			}
			break;
		case CSHELL:
			if (vflag)
				shprintf("%s %s %s",
				    (tp->flag & SPEC_BI) ? " special" : null,
				    "shell", Tbuiltin);
			break;
		case CTALIAS:
		case CEXEC:
			if (tp->flag & ISSET) {
				if (vflag) {
					shf_puts(" is ", shl_stdout);
					if (tp->type == CTALIAS)
						shprintf("a tracked %s%s for ",
						    (tp->flag & EXPORT) ?
						    "exported " : null,
						    Talias);
				}
				shf_puts(tp->val.s, shl_stdout);
			} else {
				if (vflag)
					shprintf(" %s\n", "not found");
				rv = 1;
			}
			break;
		default:
			shprintf("%s is *GOK*", id);
			break;
		}
		if (vflag || !rv)
			shf_putc('\n', shl_stdout);
	}
	return (rv);
}

/* Deal with command -vV - command -p dealt with in comexec() */
int
c_command(const char **wp)
{
	/*
	 * Let c_whence do the work. Note that c_command() must be
	 * a distinct function from c_whence() (tested in comexec()).
	 */
	return (c_whence(wp));
}

/* typeset, global, export, and readonly */
int
c_typeset(const char **wp)
{
	struct block *l;
	struct tbl *vp, **p;
	uint32_t fset = 0, fclr = 0, flag;
	int thing = 0, field, base, optc;
	const char *opts;
	const char *fieldstr, *basestr;
	bool localv = false, func = false, pflag = false, istset = true;

	switch (**wp) {

	/* export */
	case 'e':
		fset |= EXPORT;
		istset = false;
		break;

	/* readonly */
	case 'r':
		fset |= RDONLY;
		istset = false;
		break;

	/* set */
	case 's':
		/* called with 'typeset -' */
		break;

	/* typeset */
	case 't':
		localv = true;
		break;
	}

	/* see comment below regarding possible opions */
	opts = istset ? "L#R#UZ#afi#lnprtux" : "p";

	fieldstr = basestr = NULL;
	builtin_opt.flags |= GF_PLUSOPT;
	/*
	 * AT&T ksh seems to have 0-9 as options which are multiplied
	 * to get a number that is used with -L, -R, -Z or -i (eg, -1R2
	 * sets right justify in a field of 12). This allows options
	 * to be grouped in an order (eg, -Lu12), but disallows -i8 -L3 and
	 * does not allow the number to be specified as a separate argument
	 * Here, the number must follow the RLZi option, but is optional
	 * (see the # kludge in ksh_getopt()).
	 */
	while ((optc = ksh_getopt(wp, &builtin_opt, opts)) != -1) {
		flag = 0;
		switch (optc) {
		case 'L':
			flag = LJUST;
			fieldstr = builtin_opt.optarg;
			break;
		case 'R':
			flag = RJUST;
			fieldstr = builtin_opt.optarg;
			break;
		case 'U':
			/*
			 * AT&T ksh uses u, but this conflicts with
			 * upper/lower case. If this option is changed,
			 * need to change the -U below as well
			 */
			flag = INT_U;
			break;
		case 'Z':
			flag = ZEROFIL;
			fieldstr = builtin_opt.optarg;
			break;
		case 'a':
			/*
			 * this is supposed to set (-a) or unset (+a) the
			 * indexed array attribute; it does nothing on an
			 * existing regular string or indexed array though
			 */
			break;
		case 'f':
			func = true;
			break;
		case 'i':
			flag = INTEGER;
			basestr = builtin_opt.optarg;
			break;
		case 'l':
			flag = LCASEV;
			break;
		case 'n':
			set_refflag = (builtin_opt.info & GI_PLUS) ?
			    SRF_DISABLE : SRF_ENABLE;
			break;
		/* export, readonly: POSIX -p flag */
		case 'p':
			/* typeset: show values as well */
			pflag = true;
			if (istset)
				continue;
			break;
		case 'r':
			flag = RDONLY;
			break;
		case 't':
			flag = TRACE;
			break;
		case 'u':
			/* upper case / autoload */
			flag = UCASEV_AL;
			break;
		case 'x':
			flag = EXPORT;
			break;
		case '?':
			return (1);
		}
		if (builtin_opt.info & GI_PLUS) {
			fclr |= flag;
			fset &= ~flag;
			thing = '+';
		} else {
			fset |= flag;
			fclr &= ~flag;
			thing = '-';
		}
	}

	field = 0;
	if (fieldstr && !bi_getn(fieldstr, &field))
		return (1);
	base = 0;
	if (basestr && !bi_getn(basestr, &base))
		return (1);

	if (!(builtin_opt.info & GI_MINUSMINUS) && wp[builtin_opt.optind] &&
	    (wp[builtin_opt.optind][0] == '-' ||
	    wp[builtin_opt.optind][0] == '+') &&
	    wp[builtin_opt.optind][1] == '\0') {
		thing = wp[builtin_opt.optind][0];
		builtin_opt.optind++;
	}

	if (func && (((fset|fclr) & ~(TRACE|UCASEV_AL|EXPORT)) ||
	    set_refflag != SRF_NOP)) {
		bi_errorf("only -t, -u and -x options may be used with -f");
		set_refflag = SRF_NOP;
		return (1);
	}
	if (wp[builtin_opt.optind]) {
		/*
		 * Take care of exclusions.
		 * At this point, flags in fset are cleared in fclr and vice
		 * versa. This property should be preserved.
		 */
		if (fset & LCASEV)
			/* LCASEV has priority over UCASEV_AL */
			fset &= ~UCASEV_AL;
		if (fset & LJUST)
			/* LJUST has priority over RJUST */
			fset &= ~RJUST;
		if ((fset & (ZEROFIL|LJUST)) == ZEROFIL) {
			/* -Z implies -ZR */
			fset |= RJUST;
			fclr &= ~RJUST;
		}
		/*
		 * Setting these attributes clears the others, unless they
		 * are also set in this command
		 */
		if ((fset & (LJUST | RJUST | ZEROFIL | UCASEV_AL | LCASEV |
		    INTEGER | INT_U | INT_L)) || set_refflag != SRF_NOP)
			fclr |= ~fset & (LJUST | RJUST | ZEROFIL | UCASEV_AL |
			    LCASEV | INTEGER | INT_U | INT_L);
	}

	/* set variables and attributes */
	if (wp[builtin_opt.optind]) {
		int i, rv = 0;
		struct tbl *f;

		if (localv && !func)
			fset |= LOCAL;
		for (i = builtin_opt.optind; wp[i]; i++) {
			if (func) {
				f = findfunc(wp[i], hash(wp[i]),
				    tobool(fset & UCASEV_AL));
				if (!f) {
					/* AT&T ksh does ++rv: bogus */
					rv = 1;
					continue;
				}
				if (fset | fclr) {
					f->flag |= fset;
					f->flag &= ~fclr;
				} else {
					fpFUNCTf(shl_stdout, 0,
					    tobool(f->flag & FKSH),
					    wp[i], f->val.t);
					shf_putc('\n', shl_stdout);
				}
			} else if (!typeset(wp[i], fset, fclr, field, base)) {
				bi_errorf("%s: %s", wp[i], "not identifier");
				set_refflag = SRF_NOP;
				return (1);
			}
		}
		set_refflag = SRF_NOP;
		return (rv);
	}

	/* list variables and attributes */

	/* no difference at this point.. */
	flag = fset | fclr;
	if (func) {
		for (l = e->loc; l; l = l->next) {
			for (p = ktsort(&l->funs); (vp = *p++); ) {
				if (flag && (vp->flag & flag) == 0)
					continue;
				if (thing == '-')
					fpFUNCTf(shl_stdout, 0,
					    tobool(vp->flag & FKSH),
					    vp->name, vp->val.t);
				else
					shf_puts(vp->name, shl_stdout);
				shf_putc('\n', shl_stdout);
			}
		}
	} else {
		for (l = e->loc; l; l = l->next) {
			for (p = ktsort(&l->vars); (vp = *p++); ) {
				struct tbl *tvp;
				bool any_set = false;
				/*
				 * See if the parameter is set (for arrays, if any
				 * element is set).
				 */
				for (tvp = vp; tvp; tvp = tvp->u.array)
					if (tvp->flag & ISSET) {
						any_set = true;
						break;
					}

				/*
				 * Check attributes - note that all array elements
				 * have (should have?) the same attributes, so checking
				 * the first is sufficient.
				 *
				 * Report an unset param only if the user has
				 * explicitly given it some attribute (like export);
				 * otherwise, after "echo $FOO", we would report FOO...
				 */
				if (!any_set && !(vp->flag & USERATTRIB))
					continue;
				if (flag && (vp->flag & flag) == 0)
					continue;
				for (; vp; vp = vp->u.array) {
					/*
					 * Ignore array elements that aren't
					 * set unless there are no set elements,
					 * in which case the first is reported on
					 */
					if ((vp->flag&ARRAY) && any_set &&
					    !(vp->flag & ISSET))
						continue;
					/* no arguments */
					if (thing == 0 && flag == 0) {
						/*
						 * AT&T ksh prints things
						 * like export, integer,
						 * leftadj, zerofill, etc.,
						 * but POSIX says must
						 * be suitable for re-entry...
						 */
						shf_puts("typeset ", shl_stdout);
						if (((vp->flag&(ARRAY|ASSOC))==ASSOC))
							shprintf("%s ", "-n");
						if ((vp->flag&INTEGER))
							shprintf("%s ", "-i");
						if ((vp->flag&EXPORT))
							shprintf("%s ", "-x");
						if ((vp->flag&RDONLY))
							shprintf("%s ", "-r");
						if ((vp->flag&TRACE))
							shprintf("%s ", "-t");
						if ((vp->flag&LJUST))
							shprintf("-L%d ", vp->u2.field);
						if ((vp->flag&RJUST))
							shprintf("-R%d ", vp->u2.field);
						if ((vp->flag&ZEROFIL))
							shprintf("%s ", "-Z");
						if ((vp->flag&LCASEV))
							shprintf("%s ", "-l");
						if ((vp->flag&UCASEV_AL))
							shprintf("%s ", "-u");
						if ((vp->flag&INT_U))
							shprintf("%s ", "-U");
						shf_puts(vp->name, shl_stdout);
						if (pflag) {
							char *s = str_val(vp);

							shf_putc('=', shl_stdout);
							/*
							 * AT&T ksh can't have
							 * justified integers...
							 */
							if ((vp->flag &
							    (INTEGER|LJUST|RJUST)) ==
							    INTEGER)
								shf_puts(s, shl_stdout);
							else
								print_value_quoted(s);
						}
						shf_putc('\n', shl_stdout);
						if (vp->flag & ARRAY)
							break;
					} else {
						if (pflag)
							shf_puts(istset ?
							    "typeset " :
							    (flag & EXPORT) ?
							    "export " :
							    "readonly ",
							    shl_stdout);
						if ((vp->flag&ARRAY) && any_set)
							shprintf("%s[%lu]",
							    vp->name,
							    arrayindex(vp));
						else
							shf_puts(vp->name, shl_stdout);
						if (thing == '-' && (vp->flag&ISSET)) {
							char *s = str_val(vp);

							shf_putc('=', shl_stdout);
							/*
							 * AT&T ksh can't have
							 * justified integers...
							 */
							if ((vp->flag &
							    (INTEGER|LJUST|RJUST)) ==
							    INTEGER)
								shf_puts(s, shl_stdout);
							else
								print_value_quoted(s);
						}
						shf_putc('\n', shl_stdout);
					}
					/*
					 * Only report first 'element' of an array with
					 * no set elements.
					 */
					if (!any_set)
						break;
				}
			}
		}
	}
	return (0);
}

int
c_alias(const char **wp)
{
	struct table *t = &aliases;
	int rv = 0, prefix = 0;
	bool rflag = false, tflag, Uflag = false, pflag = false;
	uint32_t xflag = 0;
	int optc;

	builtin_opt.flags |= GF_PLUSOPT;
	while ((optc = ksh_getopt(wp, &builtin_opt, "dprtUx")) != -1) {
		prefix = builtin_opt.info & GI_PLUS ? '+' : '-';
		switch (optc) {
		case 'd':
#ifdef MKSH_NOPWNAM
			t = NULL;	/* fix "alias -dt" */
#else
			t = &homedirs;
#endif
			break;
		case 'p':
			pflag = true;
			break;
		case 'r':
			rflag = true;
			break;
		case 't':
			t = &taliases;
			break;
		case 'U':
			/*
			 * kludge for tracked alias initialization
			 * (don't do a path search, just make an entry)
			 */
			Uflag = true;
			break;
		case 'x':
			xflag = EXPORT;
			break;
		case '?':
			return (1);
		}
	}
#ifdef MKSH_NOPWNAM
	if (t == NULL)
		return (0);
#endif
	wp += builtin_opt.optind;

	if (!(builtin_opt.info & GI_MINUSMINUS) && *wp &&
	    (wp[0][0] == '-' || wp[0][0] == '+') && wp[0][1] == '\0') {
		prefix = wp[0][0];
		wp++;
	}

	tflag = t == &taliases;

	/* "hash -r" means reset all the tracked aliases.. */
	if (rflag) {
		static const char *args[] = {
			Tunalias, "-ta", NULL
		};

		if (!tflag || *wp) {
			shprintf("%s: -r flag can only be used with -t"
			    " and without arguments\n", Talias);
			return (1);
		}
		ksh_getopt_reset(&builtin_opt, GF_ERROR);
		return (c_unalias(args));
	}

	if (*wp == NULL) {
		struct tbl *ap, **p;

		for (p = ktsort(t); (ap = *p++) != NULL; )
			if ((ap->flag & (ISSET|xflag)) == (ISSET|xflag)) {
				if (pflag)
					shprintf("%s ", Talias);
				shf_puts(ap->name, shl_stdout);
				if (prefix != '+') {
					shf_putc('=', shl_stdout);
					print_value_quoted(ap->val.s);
				}
				shf_putc('\n', shl_stdout);
			}
	}

	for (; *wp != NULL; wp++) {
		const char *alias = *wp, *val, *newval;
		char *xalias = NULL;
		struct tbl *ap;
		uint32_t h;

		if ((val = cstrchr(alias, '='))) {
			strndupx(xalias, alias, val++ - alias, ATEMP);
			alias = xalias;
		}
		h = hash(alias);
		if (val == NULL && !tflag && !xflag) {
			ap = ktsearch(t, alias, h);
			if (ap != NULL && (ap->flag&ISSET)) {
				if (pflag)
					shprintf("%s ", Talias);
				shf_puts(ap->name, shl_stdout);
				if (prefix != '+') {
					shf_putc('=', shl_stdout);
					print_value_quoted(ap->val.s);
				}
				shf_putc('\n', shl_stdout);
			} else {
				shprintf("%s %s %s\n", alias, Talias,
				    "not found");
				rv = 1;
			}
			continue;
		}
		ap = ktenter(t, alias, h);
		ap->type = tflag ? CTALIAS : CALIAS;
		/* Are we setting the value or just some flags? */
		if ((val && !tflag) || (!val && tflag && !Uflag)) {
			if (ap->flag&ALLOC) {
				ap->flag &= ~(ALLOC|ISSET);
				afree(ap->val.s, APERM);
			}
			/* ignore values for -t (AT&T ksh does this) */
			newval = tflag ?
			    search_path(alias, path, X_OK, NULL) :
			    val;
			if (newval) {
				strdupx(ap->val.s, newval, APERM);
				ap->flag |= ALLOC|ISSET;
			} else
				ap->flag &= ~ISSET;
		}
		ap->flag |= DEFINED;
		if (prefix == '+')
			ap->flag &= ~xflag;
		else
			ap->flag |= xflag;
		afree(xalias, ATEMP);
	}

	return (rv);
}

int
c_unalias(const char **wp)
{
	struct table *t = &aliases;
	struct tbl *ap;
	int optc, rv = 0;
	bool all = false;

	while ((optc = ksh_getopt(wp, &builtin_opt, "adt")) != -1)
		switch (optc) {
		case 'a':
			all = true;
			break;
		case 'd':
#ifdef MKSH_NOPWNAM
			/* fix "unalias -dt" */
			t = NULL;
#else
			t = &homedirs;
#endif
			break;
		case 't':
			t = &taliases;
			break;
		case '?':
			return (1);
		}
#ifdef MKSH_NOPWNAM
	if (t == NULL)
		return (0);
#endif
	wp += builtin_opt.optind;

	for (; *wp != NULL; wp++) {
		ap = ktsearch(t, *wp, hash(*wp));
		if (ap == NULL) {
			/* POSIX */
			rv = 1;
			continue;
		}
		if (ap->flag&ALLOC) {
			ap->flag &= ~(ALLOC|ISSET);
			afree(ap->val.s, APERM);
		}
		ap->flag &= ~(DEFINED|ISSET|EXPORT);
	}

	if (all) {
		struct tstate ts;

		for (ktwalk(&ts, t); (ap = ktnext(&ts)); ) {
			if (ap->flag&ALLOC) {
				ap->flag &= ~(ALLOC|ISSET);
				afree(ap->val.s, APERM);
			}
			ap->flag &= ~(DEFINED|ISSET|EXPORT);
		}
	}

	return (rv);
}

int
c_let(const char **wp)
{
	int rv = 1;
	mksh_ari_t val;

	if (wp[1] == NULL)
		/* AT&T ksh does this */
		bi_errorf("no arguments");
	else
		for (wp++; *wp; wp++)
			if (!evaluate(*wp, &val, KSH_RETURN_ERROR, true)) {
				/* distinguish error from zero result */
				rv = 2;
				break;
			} else
				rv = val == 0;
	return (rv);
}

int
c_jobs(const char **wp)
{
	int optc, flag = 0, nflag = 0, rv = 0;

	while ((optc = ksh_getopt(wp, &builtin_opt, "lpnz")) != -1)
		switch (optc) {
		case 'l':
			flag = 1;
			break;
		case 'p':
			flag = 2;
			break;
		case 'n':
			nflag = 1;
			break;
		case 'z':
			/* debugging: print zombies */
			nflag = -1;
			break;
		case '?':
			return (1);
		}
	wp += builtin_opt.optind;
	if (!*wp) {
		if (j_jobs(NULL, flag, nflag))
			rv = 1;
	} else {
		for (; *wp; wp++)
			if (j_jobs(*wp, flag, nflag))
				rv = 1;
	}
	return (rv);
}

#ifndef MKSH_UNEMPLOYED
int
c_fgbg(const char **wp)
{
	bool bg = strcmp(*wp, "bg") == 0;
	int rv = 0;

	if (!Flag(FMONITOR)) {
		bi_errorf("job control not enabled");
		return (1);
	}
	if (ksh_getopt(wp, &builtin_opt, null) == '?')
		return (1);
	wp += builtin_opt.optind;
	if (*wp)
		for (; *wp; wp++)
			rv = j_resume(*wp, bg);
	else
		rv = j_resume("%%", bg);
	return (bg ? 0 : rv);
}
#endif

/* format a single kill item */
static char *
kill_fmt_entry(char *buf, size_t buflen, int i, const void *arg)
{
	const struct kill_info *ki = (const struct kill_info *)arg;

	i++;
	shf_snprintf(buf, buflen, "%*d %*s %s",
	    ki->num_width, i,
	    ki->name_width, sigtraps[i].name,
	    sigtraps[i].mess);
	return (buf);
}

int
c_kill(const char **wp)
{
	Trap *t = NULL;
	const char *p;
	bool lflag = false;
	int i, n, rv, sig;

	/* assume old style options if -digits or -UPPERCASE */
	if ((p = wp[1]) && *p == '-' && (ksh_isdigit(p[1]) ||
	    ksh_isupper(p[1]))) {
		if (!(t = gettrap(p + 1, true))) {
			bi_errorf("bad signal '%s'", p + 1);
			return (1);
		}
		i = (wp[2] && strcmp(wp[2], "--") == 0) ? 3 : 2;
	} else {
		int optc;

		while ((optc = ksh_getopt(wp, &builtin_opt, "ls:")) != -1)
			switch (optc) {
			case 'l':
				lflag = true;
				break;
			case 's':
				if (!(t = gettrap(builtin_opt.optarg, true))) {
					bi_errorf("bad signal '%s'",
					    builtin_opt.optarg);
					return (1);
				}
				break;
			case '?':
				return (1);
			}
		i = builtin_opt.optind;
	}
	if ((lflag && t) || (!wp[i] && !lflag)) {
#ifndef MKSH_SMALL
		shf_puts("usage:\tkill [-s signame | -signum | -signame]"
		    " { job | pid | pgrp } ...\n"
		    "\tkill -l [exit_status ...]\n", shl_out);
#endif
		bi_errorfz();
		return (1);
	}

	if (lflag) {
		if (wp[i]) {
			for (; wp[i]; i++) {
				if (!bi_getn(wp[i], &n))
					return (1);
				if (n > 128 && n < 128 + NSIG)
					n -= 128;
				if (n > 0 && n < NSIG)
					shprintf("%s\n", sigtraps[n].name);
				else
					shprintf("%d\n", n);
			}
		} else {
			ssize_t w, mess_cols, mess_octs;
			int j;
			struct kill_info ki;

			for (j = NSIG, ki.num_width = 1; j >= 10; j /= 10)
				ki.num_width++;
			ki.name_width = mess_cols = mess_octs = 0;
			for (j = 0; j < NSIG; j++) {
				w = strlen(sigtraps[j].name);
				if (w > ki.name_width)
					ki.name_width = w;
				w = strlen(sigtraps[j].mess);
				if (w > mess_octs)
					mess_octs = w;
				w = utf_mbswidth(sigtraps[j].mess);
				if (w > mess_cols)
					mess_cols = w;
			}

			print_columns(shl_stdout, NSIG - 1,
			    kill_fmt_entry, (void *)&ki,
			    ki.num_width + 1 + ki.name_width + 1 + mess_octs,
			    ki.num_width + 1 + ki.name_width + 1 + mess_cols,
			    true);
		}
		return (0);
	}
	rv = 0;
	sig = t ? t->signal : SIGTERM;
	for (; (p = wp[i]); i++) {
		if (*p == '%') {
			if (j_kill(p, sig))
				rv = 1;
		} else if (!getn(p, &n)) {
			bi_errorf("%s: %s", p,
			    "arguments must be jobs or process IDs");
			rv = 1;
		} else {
			if (mksh_kill(n, sig) < 0) {
				bi_errorf("%s: %s", p, strerror(errno));
				rv = 1;
			}
		}
	}
	return (rv);
}

void
getopts_reset(int val)
{
	if (val >= 1) {
		ksh_getopt_reset(&user_opt, GF_NONAME | GF_PLUSOPT);
		user_opt.optind = user_opt.uoptind = val;
	}
}

int
c_getopts(const char **wp)
{
	int argc, optc, rv;
	const char *opts, *var;
	char buf[3];
	struct tbl *vq, *voptarg;

	if (ksh_getopt(wp, &builtin_opt, null) == '?')
		return (1);
	wp += builtin_opt.optind;

	opts = *wp++;
	if (!opts) {
		bi_errorf("missing %s argument", "options");
		return (1);
	}

	var = *wp++;
	if (!var) {
		bi_errorf("missing %s argument", "name");
		return (1);
	}
	if (!*var || *skip_varname(var, true)) {
		bi_errorf("%s: %s", var, "is not an identifier");
		return (1);
	}

	if (e->loc->next == NULL) {
		internal_warningf("%s: %s", "c_getopts", "no argv");
		return (1);
	}
	/* Which arguments are we parsing... */
	if (*wp == NULL)
		wp = e->loc->next->argv;
	else
		*--wp = e->loc->next->argv[0];

	/* Check that our saved state won't cause a core dump... */
	for (argc = 0; wp[argc]; argc++)
		;
	if (user_opt.optind > argc ||
	    (user_opt.p != 0 &&
	    user_opt.p > strlen(wp[user_opt.optind - 1]))) {
		bi_errorf("arguments changed since last call");
		return (1);
	}

	user_opt.optarg = NULL;
	optc = ksh_getopt(wp, &user_opt, opts);

	if (optc >= 0 && optc != '?' && (user_opt.info & GI_PLUS)) {
		buf[0] = '+';
		buf[1] = optc;
		buf[2] = '\0';
	} else {
		/*
		 * POSIX says var is set to ? at end-of-options, AT&T ksh
		 * sets it to null - we go with POSIX...
		 */
		buf[0] = optc < 0 ? '?' : optc;
		buf[1] = '\0';
	}

	/* AT&T ksh93 in fact does change OPTIND for unknown options too */
	user_opt.uoptind = user_opt.optind;

	voptarg = global("OPTARG");
	/* AT&T ksh clears ro and int */
	voptarg->flag &= ~RDONLY;
	/* Paranoia: ensure no bizarre results. */
	if (voptarg->flag & INTEGER)
	    typeset("OPTARG", 0, INTEGER, 0, 0);
	if (user_opt.optarg == NULL)
		unset(voptarg, 1);
	else
		/* This can't fail (have cleared readonly/integer) */
		setstr(voptarg, user_opt.optarg, KSH_RETURN_ERROR);

	rv = 0;

	vq = global(var);
	/* Error message already printed (integer, readonly) */
	if (!setstr(vq, buf, KSH_RETURN_ERROR))
		rv = 2;
	if (Flag(FEXPORT))
		typeset(var, EXPORT, 0, 0, 0);

	return (optc < 0 ? 1 : rv);
}

int
c_bind(const char **wp)
{
	int optc, rv = 0;
#ifndef MKSH_SMALL
	bool macro = false;
#endif
	bool list = false;
	const char *cp;
	char *up;

	while ((optc = ksh_getopt(wp, &builtin_opt,
#ifndef MKSH_SMALL
	    "lm"
#else
	    "l"
#endif
	    )) != -1)
		switch (optc) {
		case 'l':
			list = true;
			break;
#ifndef MKSH_SMALL
		case 'm':
			macro = true;
			break;
#endif
		case '?':
			return (1);
		}
	wp += builtin_opt.optind;

	if (*wp == NULL)
		/* list all */
		rv = x_bind(NULL, NULL,
#ifndef MKSH_SMALL
		    false,
#endif
		    list);

	for (; *wp != NULL; wp++) {
		if ((cp = cstrchr(*wp, '=')) == NULL)
			up = NULL;
		else {
			strdupx(up, *wp, ATEMP);
			up[cp++ - *wp] = '\0';
		}
		if (x_bind(up ? up : *wp, cp,
#ifndef MKSH_SMALL
		    macro,
#endif
		    false))
			rv = 1;
		afree(up, ATEMP);
	}

	return (rv);
}

int
c_shift(const char **wp)
{
	struct block *l = e->loc;
	int n;
	mksh_ari_t val;
	const char *arg;

	if (ksh_getopt(wp, &builtin_opt, null) == '?')
		return (1);
	arg = wp[builtin_opt.optind];

	if (arg) {
		evaluate(arg, &val, KSH_UNWIND_ERROR, false);
		n = val;
	} else
		n = 1;
	if (n < 0) {
		bi_errorf("%s: %s", arg, "bad number");
		return (1);
	}
	if (l->argc < n) {
		bi_errorf("nothing to shift");
		return (1);
	}
	l->argv[n] = l->argv[0];
	l->argv += n;
	l->argc -= n;
	return (0);
}

int
c_umask(const char **wp)
{
	int i, optc;
	const char *cp;
	bool symbolic = false;
	mode_t old_umask;

	while ((optc = ksh_getopt(wp, &builtin_opt, "S")) != -1)
		switch (optc) {
		case 'S':
			symbolic = true;
			break;
		case '?':
			return (1);
		}
	cp = wp[builtin_opt.optind];
	if (cp == NULL) {
		old_umask = umask((mode_t)0);
		umask(old_umask);
		if (symbolic) {
			char buf[18], *p;
			int j;

			old_umask = ~old_umask;
			p = buf;
			for (i = 0; i < 3; i++) {
				*p++ = "ugo"[i];
				*p++ = '=';
				for (j = 0; j < 3; j++)
					if (old_umask & (1 << (8 - (3*i + j))))
						*p++ = "rwx"[j];
				*p++ = ',';
			}
			p[-1] = '\0';
			shprintf("%s\n", buf);
		} else
			shprintf("%#3.3o\n", (unsigned int)old_umask);
	} else {
		mode_t new_umask;

		if (ksh_isdigit(*cp)) {
			for (new_umask = 0; *cp >= '0' && *cp <= '7'; cp++)
				new_umask = new_umask * 8 + (*cp - '0');
			if (*cp) {
				bi_errorf("bad number");
				return (1);
			}
		} else {
			/* symbolic format */
			int positions, new_val;
			char op;

			old_umask = umask((mode_t)0);
			/* in case of error */
			umask(old_umask);
			old_umask = ~old_umask;
			new_umask = old_umask;
			positions = 0;
			while (*cp) {
				while (*cp && vstrchr("augo", *cp))
					switch (*cp++) {
					case 'a':
						positions |= 0111;
						break;
					case 'u':
						positions |= 0100;
						break;
					case 'g':
						positions |= 0010;
						break;
					case 'o':
						positions |= 0001;
						break;
					}
				if (!positions)
					/* default is a */
					positions = 0111;
				if (!vstrchr("=+-", op = *cp))
					break;
				cp++;
				new_val = 0;
				while (*cp && vstrchr("rwxugoXs", *cp))
					switch (*cp++) {
					case 'r': new_val |= 04; break;
					case 'w': new_val |= 02; break;
					case 'x': new_val |= 01; break;
					case 'u':
						new_val |= old_umask >> 6;
						break;
					case 'g':
						new_val |= old_umask >> 3;
						break;
					case 'o':
						new_val |= old_umask >> 0;
						break;
					case 'X':
						if (old_umask & 0111)
							new_val |= 01;
						break;
					case 's':
						/* ignored */
						break;
					}
				new_val = (new_val & 07) * positions;
				switch (op) {
				case '-':
					new_umask &= ~new_val;
					break;
				case '=':
					new_umask = new_val |
					    (new_umask & ~(positions * 07));
					break;
				case '+':
					new_umask |= new_val;
				}
				if (*cp == ',') {
					positions = 0;
					cp++;
				} else if (!vstrchr("=+-", *cp))
					break;
			}
			if (*cp) {
				bi_errorf("bad mask");
				return (1);
			}
			new_umask = ~new_umask;
		}
		umask(new_umask);
	}
	return (0);
}

int
c_dot(const char **wp)
{
	const char *file, *cp, **argv;
	int argc, i, errcode;

	if (ksh_getopt(wp, &builtin_opt, null) == '?')
		return (1);

	if ((cp = wp[builtin_opt.optind]) == NULL) {
		bi_errorf("missing argument");
		return (1);
	}
	if ((file = search_path(cp, path, R_OK, &errcode)) == NULL) {
		bi_errorf("%s: %s", cp, strerror(errcode));
		return (1);
	}

	/* Set positional parameters? */
	if (wp[builtin_opt.optind + 1]) {
		argv = wp + builtin_opt.optind;
		/* preserve $0 */
		argv[0] = e->loc->argv[0];
		for (argc = 0; argv[argc + 1]; argc++)
			;
	} else {
		argc = 0;
		argv = NULL;
	}
	if ((i = include(file, argc, argv, 0)) < 0) {
		/* should not happen */
		bi_errorf("%s: %s", cp, strerror(errno));
		return (1);
	}
	return (i);
}

int
c_wait(const char **wp)
{
	int rv = 0, sig;

	if (ksh_getopt(wp, &builtin_opt, null) == '?')
		return (1);
	wp += builtin_opt.optind;
	if (*wp == NULL) {
		while (waitfor(NULL, &sig) >= 0)
			;
		rv = sig;
	} else {
		for (; *wp; wp++)
			rv = waitfor(*wp, &sig);
		if (rv < 0)
			/* magic exit code: bad job-id */
			rv = sig ? sig : 127;
	}
	return (rv);
}

int
c_read(const char **wp)
{
#define is_ifsws(c) (ctype((c), C_IFS) && ctype((c), C_IFSWS))
	static char REPLY[] = "REPLY";
	int c, fd = 0, rv = 0, lastparm = 0;
	bool savehist = false, intoarray = false, aschars = false;
	bool rawmode = false, expanding = false;
	enum { LINES, BYTES, UPTO, READALL } readmode = LINES;
	char delim = '\n';
	size_t bytesleft = 128, bytesread;
	struct tbl *vp /* FU gcc */ = NULL, *vq;
	char *cp, *allocd = NULL, *xp;
	const char *ccp;
	XString xs;
	ptrdiff_t xsave = 0;
	struct termios tios;
	bool restore_tios = false;
#if HAVE_SELECT
	bool hastimeout = false;
	struct timeval tv, tvlim;
#define c_read_opts "Aad:N:n:prst:u,"
#else
#define c_read_opts "Aad:N:n:prsu,"
#endif

	while ((c = ksh_getopt(wp, &builtin_opt, c_read_opts)) != -1)
	switch (c) {
	case 'a':
		aschars = true;
		/* FALLTHROUGH */
	case 'A':
		intoarray = true;
		break;
	case 'd':
		delim = builtin_opt.optarg[0];
		break;
	case 'N':
	case 'n':
		readmode = c == 'N' ? BYTES : UPTO;
		if (!bi_getn(builtin_opt.optarg, &c))
			return (2);
		if (c == -1) {
			readmode = READALL;
			bytesleft = 1024;
		} else
			bytesleft = (unsigned int)c;
		break;
	case 'p':
		if ((fd = coproc_getfd(R_OK, &ccp)) < 0) {
			bi_errorf("%s: %s", "-p", ccp);
			return (2);
		}
		break;
	case 'r':
		rawmode = true;
		break;
	case 's':
		savehist = true;
		break;
#if HAVE_SELECT
	case 't':
		if (parse_usec(builtin_opt.optarg, &tv)) {
			bi_errorf("%s: %s '%s'", Tsynerr, strerror(errno),
			    builtin_opt.optarg);
			return (2);
		}
		hastimeout = true;
		break;
#endif
	case 'u':
		if (!builtin_opt.optarg[0])
			fd = 0;
		else if ((fd = check_fd(builtin_opt.optarg, R_OK, &ccp)) < 0) {
			bi_errorf("%s: %s: %s", "-u", builtin_opt.optarg, ccp);
			return (2);
		}
		break;
	case '?':
		return (2);
	}
	wp += builtin_opt.optind;
	if (*wp == NULL)
		*--wp = REPLY;

	if (intoarray && wp[1] != NULL) {
		bi_errorf("too many arguments");
		return (2);
	}

	if ((ccp = cstrchr(*wp, '?')) != NULL) {
		strdupx(allocd, *wp, ATEMP);
		allocd[ccp - *wp] = '\0';
		*wp = allocd;
		if (isatty(fd)) {
			/*
			 * AT&T ksh says it prints prompt on fd if it's open
			 * for writing and is a tty, but it doesn't do it
			 * (it also doesn't check the interactive flag,
			 * as is indicated in the Korn Shell book).
			 */
			shf_puts(ccp + 1, shl_out);
			shf_flush(shl_out);
		}
	}

	Xinit(xs, xp, bytesleft, ATEMP);

	if (readmode == LINES)
		bytesleft = 1;
	else if (isatty(fd)) {
		x_mkraw(fd, &tios, true);
		restore_tios = true;
	}

#if HAVE_SELECT
	if (hastimeout) {
		gettimeofday(&tvlim, NULL);
		timeradd(&tvlim, &tv, &tvlim);
	}
#endif

 c_read_readloop:
#if HAVE_SELECT
	if (hastimeout) {
		fd_set fdset;

		FD_ZERO(&fdset);
		FD_SET(fd, &fdset);
		gettimeofday(&tv, NULL);
		timersub(&tvlim, &tv, &tv);
		if (tv.tv_sec < 0) {
			/* timeout expired globally */
			rv = 1;
			goto c_read_out;
		}

		switch (select(fd + 1, &fdset, NULL, NULL, &tv)) {
		case 1:
			break;
		case 0:
			/* timeout expired for this call */
			rv = 1;
			goto c_read_out;
		default:
			bi_errorf("%s: %s", Tselect, strerror(errno));
			rv = 2;
			goto c_read_out;
		}
	}
#endif

	bytesread = blocking_read(fd, xp, bytesleft);
	if (bytesread == (size_t)-1) {
		/* interrupted */
		if (errno == EINTR && fatal_trap_check()) {
			/*
			 * Was the offending signal one that would
			 * normally kill a process? If so, pretend
			 * the read was killed.
			 */
			rv = 2;
			goto c_read_out;
		}
		/* just ignore the signal */
		goto c_read_readloop;
	}

	switch (readmode) {
	case READALL:
		if (bytesread == 0) {
			/* end of file reached */
			rv = 1;
			goto c_read_readdone;
		}
		xp += bytesread;
		XcheckN(xs, xp, bytesleft);
		break;

	case UPTO:
		if (bytesread == 0)
			/* end of file reached */
			rv = 1;
		xp += bytesread;
		goto c_read_readdone;

	case BYTES:
		if (bytesread == 0) {
			/* end of file reached */
			rv = 1;
			xp = Xstring(xs, xp);
			goto c_read_readdone;
		}
		xp += bytesread;
		if ((bytesleft -= bytesread) == 0)
			goto c_read_readdone;
		break;
	case LINES:
		if (bytesread == 0) {
			/* end of file reached */
			rv = 1;
			goto c_read_readdone;
		}
		if ((c = *xp) == '\0' && !aschars && delim != '\0') {
			/* skip any read NULs unless delimiter */
			break;
		}
		if (expanding) {
			expanding = false;
			if (c == delim) {
				if (Flag(FTALKING_I) && isatty(fd)) {
					/*
					 * set prompt in case this is
					 * called from .profile or $ENV
					 */
					set_prompt(PS2, NULL);
					pprompt(prompt, 0);
				}
				/* drop the backslash */
				--xp;
				/* and the delimiter */
				break;
			}
		} else if (c == delim) {
			goto c_read_readdone;
		} else if (!rawmode && c == '\\') {
			expanding = true;
		}
		Xcheck(xs, xp);
		++xp;
		break;
	}
	goto c_read_readloop;

 c_read_readdone:
	bytesread = Xlength(xs, xp);
	Xput(xs, xp, '\0');

	/*-
	 * state: we finished reading the input and NUL terminated it
	 * Xstring(xs, xp) -> xp-1 = input string without trailing delim
	 * rv = 1 if EOF, 0 otherwise (errors handled already)
	 */

	if (rv == 1) {
		/* clean up coprocess if needed, on EOF */
		coproc_read_close(fd);
		if (readmode == READALL)
			/* EOF is no error here */
			rv = 0;
	}

	if (savehist)
		histsave(&source->line, Xstring(xs, xp), true, false);

	ccp = cp = Xclose(xs, xp);
	expanding = false;
	XinitN(xs, 128, ATEMP);
	if (intoarray) {
		vp = global(*wp);
		if (vp->flag & RDONLY) {
 c_read_splitro:
			bi_errorf("%s: %s", *wp, "is read only");
 c_read_spliterr:
			rv = 2;
			afree(cp, ATEMP);
			goto c_read_out;
		}
		/* exporting an array is currently pointless */
		unset(vp, 1);
		/* counter for array index */
		c = 0;
	}
	if (!aschars) {
		/* skip initial IFS whitespace */
		while (bytesread && is_ifsws(*ccp)) {
			++ccp;
			--bytesread;
		}
		/* trim trailing IFS whitespace */
		while (bytesread && is_ifsws(ccp[bytesread - 1])) {
			--bytesread;
		}
	}
 c_read_splitloop:
	xp = Xstring(xs, xp);
	/* generate next word */
	if (!bytesread) {
		/* no more input */
		if (intoarray)
			goto c_read_splitdone;
		/* zero out next parameters */
		goto c_read_gotword;
	}
	if (aschars) {
		Xput(xs, xp, '1');
		Xput(xs, xp, '#');
		bytesleft = utf_ptradj(ccp);
		while (bytesleft && bytesread) {
			*xp++ = *ccp++;
			--bytesleft;
			--bytesread;
		}
		if (xp[-1] == '\0') {
			xp[-1] = '0';
			xp[-3] = '2';
		}
		goto c_read_gotword;
	}

	if (!intoarray && wp[1] == NULL)
		lastparm = 1;

 c_read_splitlast:
	/* copy until IFS character */
	while (bytesread) {
		char ch;

		ch = *ccp;
		if (expanding) {
			expanding = false;
			goto c_read_splitcopy;
		} else if (ctype(ch, C_IFS)) {
			break;
		} else if (!rawmode && ch == '\\') {
			expanding = true;
		} else {
 c_read_splitcopy:
			Xcheck(xs, xp);
			Xput(xs, xp, ch);
		}
		++ccp;
		--bytesread;
	}
	xsave = Xsavepos(xs, xp);
	/* copy word delimiter: IFSWS+IFS,IFSWS */
	while (bytesread) {
		char ch;

		ch = *ccp;
		if (!ctype(ch, C_IFS))
			break;
		Xcheck(xs, xp);
		Xput(xs, xp, ch);
		++ccp;
		--bytesread;
		if (!ctype(ch, C_IFSWS))
			break;
	}
	while (bytesread && is_ifsws(*ccp)) {
		Xcheck(xs, xp);
		Xput(xs, xp, *ccp);
		++ccp;
		--bytesread;
	}
	/* if no more parameters, rinse and repeat */
	if (lastparm && bytesread) {
		++lastparm;
		goto c_read_splitlast;
	}
	/* get rid of the delimiter unless we pack the rest */
	if (lastparm < 2)
		xp = Xrestpos(xs, xp, xsave);
 c_read_gotword:
	Xput(xs, xp, '\0');
	if (intoarray) {
		vq = arraysearch(vp, c++);
	} else {
		vq = global(*wp);
		/* must be checked before exporting */
		if (vq->flag & RDONLY)
			goto c_read_splitro;
		if (Flag(FEXPORT))
			typeset(*wp, EXPORT, 0, 0, 0);
	}
	if (!setstr(vq, Xstring(xs, xp), KSH_RETURN_ERROR))
		goto c_read_spliterr;
	if (aschars) {
		setint_v(vq, vq, false);
		/* protect from UTFMODE changes */
		vq->type = 0;
	}
	if (intoarray || *++wp != NULL)
		goto c_read_splitloop;

 c_read_splitdone:
	/* free up */
	afree(cp, ATEMP);

 c_read_out:
	afree(allocd, ATEMP);
	Xfree(xs, xp);
	if (restore_tios)
		tcsetattr(fd, TCSADRAIN, &tios);
	return (rv);
#undef is_ifsws
}

int
c_eval(const char **wp)
{
	struct source *s, *saves = source;
	unsigned char savef;
	int rv;

	if (ksh_getopt(wp, &builtin_opt, null) == '?')
		return (1);
	s = pushs(SWORDS, ATEMP);
	s->u.strv = wp + builtin_opt.optind;

	/*-
	 * The following code handles the case where the command is
	 * empty due to failed command substitution, for example by
	 *	eval "$(false)"
	 * This has historically returned 1 by AT&T ksh88. In this
	 * case, shell() will not set or change exstat because the
	 * compiled tree is empty, so it will use the value we pass
	 * from subst_exstat, which is cleared in execute(), so it
	 * should have been 0 if there were no substitutions.
	 *
	 * POSIX however says we don't do this, even though it is
	 * traditionally done. AT&T ksh93 agrees with POSIX, so we
	 * do. The following is an excerpt from SUSv4 [1003.2-2008]:
	 *
	 * 2.9.1: Simple Commands
	 *	... If there is a command name, execution shall
	 *	continue as described in 2.9.1.1 [Command Search
	 *	and Execution]. If there is no command name, but
	 *	the command contained a command substitution, the
	 *	command shall complete with the exit status of the
	 *	last command substitution performed.
	 * 2.9.1.1: Command Search and Execution
	 *	(1) a. If the command name matches the name of a
	 *	special built-in utility, that special built-in
	 *	utility shall be invoked.
	 * 2.14.5: eval
	 *	If there are no arguments, or only null arguments,
	 *	eval shall return a zero exit status; ...
	 */
	/* exstat = subst_exstat; */	/* AT&T ksh88 */
	exstat = 0;			/* SUSv4 */

	savef = Flag(FERREXIT);
	Flag(FERREXIT) = 0;
	rv = shell(s, false);
	Flag(FERREXIT) = savef;
	source = saves;
	afree(s, ATEMP);
	return (rv);
}

int
c_trap(const char **wp)
{
	int i;
	const char *s;
	Trap *p;

	if (ksh_getopt(wp, &builtin_opt, null) == '?')
		return (1);
	wp += builtin_opt.optind;

	if (*wp == NULL) {
		for (p = sigtraps, i = NSIG+1; --i >= 0; p++)
			if (p->trap != NULL) {
				shf_puts("trap -- ", shl_stdout);
				print_value_quoted(p->trap);
				shprintf(" %s\n", p->name);
			}
		return (0);
	}

	/*
	 * Use case sensitive lookup for first arg so the
	 * command 'exit' isn't confused with the pseudo-signal
	 * 'EXIT'.
	 */
	/* get command */
	s = (gettrap(*wp, false) == NULL) ? *wp++ : NULL;
	if (s != NULL && s[0] == '-' && s[1] == '\0')
		s = NULL;

	/* set/clear traps */
	i = 0;
	while (*wp != NULL)
		if ((p = gettrap(*wp++, true)) == NULL) {
			warningf(true, "%s: %s '%s'", builtin_argv0,
			    "bad signal", wp[-1]);
			++i;
		} else
			settrap(p, s);
	return (i);
}

int
c_exitreturn(const char **wp)
{
	int n, how = LEXIT;
	const char *arg;

	if (ksh_getopt(wp, &builtin_opt, null) == '?')
		return (1);
	arg = wp[builtin_opt.optind];

	if (arg) {
		if (!getn(arg, &n)) {
			exstat = 1;
			warningf(true, "%s: %s", arg, "bad number");
		} else
			exstat = n;
	} else if (trap_exstat != -1)
		exstat = trap_exstat;
	if (wp[0][0] == 'r') {
		/* return */
		struct env *ep;

		/*
		 * need to tell if this is exit or return so trap exit will
		 * work right (POSIX)
		 */
		for (ep = e; ep; ep = ep->oenv)
			if (STOP_RETURN(ep->type)) {
				how = LRETURN;
				break;
			}
	}

	if (how == LEXIT && !really_exit && j_stopped_running()) {
		really_exit = 1;
		how = LSHELL;
	}

	/* get rid of any i/o redirections */
	quitenv(NULL);
	unwind(how);
	/* NOTREACHED */
}

int
c_brkcont(const char **wp)
{
	int n, quit;
	struct env *ep, *last_ep = NULL;
	const char *arg;

	if (ksh_getopt(wp, &builtin_opt, null) == '?')
		return (1);
	arg = wp[builtin_opt.optind];

	if (!arg)
		n = 1;
	else if (!bi_getn(arg, &n))
		return (1);
	quit = n;
	if (quit <= 0) {
		/* AT&T ksh does this for non-interactive shells only - weird */
		bi_errorf("%s: %s", arg, "bad value");
		return (1);
	}

	/* Stop at E_NONE, E_PARSE, E_FUNC, or E_INCL */
	for (ep = e; ep && !STOP_BRKCONT(ep->type); ep = ep->oenv)
		if (ep->type == E_LOOP) {
			if (--quit == 0)
				break;
			ep->flags |= EF_BRKCONT_PASS;
			last_ep = ep;
		}

	if (quit) {
		/*
		 * AT&T ksh doesn't print a message - just does what it
		 * can. We print a message 'cause it helps in debugging
		 * scripts, but don't generate an error (ie, keep going).
		 */
		if (n == quit) {
			warningf(true, "%s: %s %s", wp[0], "can't", wp[0]);
			return (0);
		}
		/*
		 * POSIX says if n is too big, the last enclosing loop
		 * shall be used. Doesn't say to print an error but we
		 * do anyway 'cause the user messed up.
		 */
		if (last_ep)
			last_ep->flags &= ~EF_BRKCONT_PASS;
		warningf(true, "%s: can only %s %d level(s)",
		    wp[0], wp[0], n - quit);
	}

	unwind(*wp[0] == 'b' ? LBREAK : LCONTIN);
	/* NOTREACHED */
}

int
c_set(const char **wp)
{
	int argi;
	bool setargs;
	struct block *l = e->loc;
	const char **owp;

	if (wp[1] == NULL) {
		static const char *args[] = { Tset, "-", NULL };
		return (c_typeset(args));
	}

	argi = parse_args(wp, OF_SET, &setargs);
	if (argi < 0)
		return (1);
	/* set $# and $* */
	if (setargs) {
		wp += argi - 1;
		owp = wp;
		/* save $0 */
		wp[0] = l->argv[0];
		while (*++wp != NULL)
			strdupx(*wp, *wp, &l->area);
		l->argc = wp - owp - 1;
		l->argv = alloc2(l->argc + 2, sizeof(char *), &l->area);
		for (wp = l->argv; (*wp++ = *owp++) != NULL; )
			;
	}
	/*-
	 * POSIX says set exit status is 0, but old scripts that use
	 * getopt(1) use the construct
	 *	set -- $(getopt ab:c "$@")
	 * which assumes the exit value set will be that of the $()
	 * (subst_exstat is cleared in execute() so that it will be 0
	 * if there are no command substitutions).
	 * Switched ksh (!posix !sh) to POSIX in mksh R39b.
	 */
	return (Flag(FSH) ? subst_exstat : 0);
}

int
c_unset(const char **wp)
{
	const char *id;
	int optc, rv = 0;
	bool unset_var = true;

	while ((optc = ksh_getopt(wp, &builtin_opt, "fv")) != -1)
		switch (optc) {
		case 'f':
			unset_var = false;
			break;
		case 'v':
			unset_var = true;
			break;
		case '?':
			/*XXX not reached due to GF_ERROR */
			return (2);
		}
	wp += builtin_opt.optind;
	for (; (id = *wp) != NULL; wp++)
		if (unset_var) {
			/* unset variable */
			struct tbl *vp;
			char *cp = NULL;
			size_t n;

			n = strlen(id);
			if (n > 3 && id[n-3] == '[' && id[n-2] == '*' &&
			    id[n-1] == ']') {
				strndupx(cp, id, n - 3, ATEMP);
				id = cp;
				optc = 3;
			} else
				optc = vstrchr(id, '[') ? 0 : 1;

			vp = global(id);
			afree(cp, ATEMP);

			if ((vp->flag&RDONLY)) {
				warningf(true, "%s: %s", vp->name,
				    "is read only");
				rv = 1;
			} else
				unset(vp, optc);
		} else
			/* unset function */
			define(id, NULL);
	return (rv);
}

static void
p_time(struct shf *shf, bool posix, long tv_sec, int tv_usec, int width,
    const char *prefix, const char *suffix)
{
	tv_usec /= 10000;
	if (posix)
		shf_fprintf(shf, "%s%*ld.%02d%s", prefix, width,
		    tv_sec, tv_usec, suffix);
	else
		shf_fprintf(shf, "%s%*ldm%d.%02ds%s", prefix, width,
		    tv_sec / 60, (int)(tv_sec % 60), tv_usec, suffix);
}

int
c_times(const char **wp MKSH_A_UNUSED)
{
	struct rusage usage;

	getrusage(RUSAGE_SELF, &usage);
	p_time(shl_stdout, false, usage.ru_utime.tv_sec,
	    usage.ru_utime.tv_usec, 0, null, " ");
	p_time(shl_stdout, false, usage.ru_stime.tv_sec,
	    usage.ru_stime.tv_usec, 0, null, "\n");

	getrusage(RUSAGE_CHILDREN, &usage);
	p_time(shl_stdout, false, usage.ru_utime.tv_sec,
	    usage.ru_utime.tv_usec, 0, null, " ");
	p_time(shl_stdout, false, usage.ru_stime.tv_sec,
	    usage.ru_stime.tv_usec, 0, null, "\n");

	return (0);
}

/*
 * time pipeline (really a statement, not a built-in command)
 */
int
timex(struct op *t, int f, volatile int *xerrok)
{
#define TF_NOARGS	BIT(0)
#define TF_NOREAL	BIT(1)		/* don't report real time */
#define TF_POSIX	BIT(2)		/* report in POSIX format */
	int rv = 0, tf = 0;
	struct rusage ru0, ru1, cru0, cru1;
	struct timeval usrtime, systime, tv0, tv1;

	gettimeofday(&tv0, NULL);
	getrusage(RUSAGE_SELF, &ru0);
	getrusage(RUSAGE_CHILDREN, &cru0);
	if (t->left) {
		/*
		 * Two ways of getting cpu usage of a command: just use t0
		 * and t1 (which will get cpu usage from other jobs that
		 * finish while we are executing t->left), or get the
		 * cpu usage of t->left. AT&T ksh does the former, while
		 * pdksh tries to do the later (the j_usrtime hack doesn't
		 * really work as it only counts the last job).
		 */
		timerclear(&j_usrtime);
		timerclear(&j_systime);
		rv = execute(t->left, f | XTIME, xerrok);
		if (t->left->type == TCOM)
			tf |= t->left->str[0];
		gettimeofday(&tv1, NULL);
		getrusage(RUSAGE_SELF, &ru1);
		getrusage(RUSAGE_CHILDREN, &cru1);
	} else
		tf = TF_NOARGS;

	if (tf & TF_NOARGS) {
		/* ksh93 - report shell times (shell+kids) */
		tf |= TF_NOREAL;
		timeradd(&ru0.ru_utime, &cru0.ru_utime, &usrtime);
		timeradd(&ru0.ru_stime, &cru0.ru_stime, &systime);
	} else {
		timersub(&ru1.ru_utime, &ru0.ru_utime, &usrtime);
		timeradd(&usrtime, &j_usrtime, &usrtime);
		timersub(&ru1.ru_stime, &ru0.ru_stime, &systime);
		timeradd(&systime, &j_systime, &systime);
	}

	if (!(tf & TF_NOREAL)) {
		timersub(&tv1, &tv0, &tv1);
		if (tf & TF_POSIX)
			p_time(shl_out, true, tv1.tv_sec, tv1.tv_usec,
			    5, "real ", "\n");
		else
			p_time(shl_out, false, tv1.tv_sec, tv1.tv_usec,
			    5, null, " real ");
	}
	if (tf & TF_POSIX)
		p_time(shl_out, true, usrtime.tv_sec, usrtime.tv_usec,
		    5, "user ", "\n");
	else
		p_time(shl_out, false, usrtime.tv_sec, usrtime.tv_usec,
		    5, null, " user ");
	if (tf & TF_POSIX)
		p_time(shl_out, true, systime.tv_sec, systime.tv_usec,
		    5, "sys  ", "\n");
	else
		p_time(shl_out, false, systime.tv_sec, systime.tv_usec,
		    5, null, " system\n");
	shf_flush(shl_out);

	return (rv);
}

void
timex_hook(struct op *t, char **volatile *app)
{
	char **wp = *app;
	int optc, i, j;
	Getopt opt;

	ksh_getopt_reset(&opt, 0);
	/* start at the start */
	opt.optind = 0;
	while ((optc = ksh_getopt((const char **)wp, &opt, ":p")) != -1)
		switch (optc) {
		case 'p':
			t->str[0] |= TF_POSIX;
			break;
		case '?':
			errorf("time: -%s %s", opt.optarg,
			    "unknown option");
		case ':':
			errorf("time: -%s %s", opt.optarg,
			    "requires an argument");
		}
	/* Copy command words down over options. */
	if (opt.optind != 0) {
		for (i = 0; i < opt.optind; i++)
			afree(wp[i], ATEMP);
		for (i = 0, j = opt.optind; (wp[i] = wp[j]); i++, j++)
			;
	}
	if (!wp[0])
		t->str[0] |= TF_NOARGS;
	*app = wp;
}

/* exec with no args - args case is taken care of in comexec() */
int
c_exec(const char **wp MKSH_A_UNUSED)
{
	int i;

	/* make sure redirects stay in place */
	if (e->savefd != NULL) {
		for (i = 0; i < NUFILE; i++) {
			if (e->savefd[i] > 0)
				close(e->savefd[i]);
			/*
			 * keep all file descriptors > 2 private for ksh,
			 * but not for POSIX or legacy/kludge sh
			 */
			if (!Flag(FPOSIX) && !Flag(FSH) && i > 2 &&
			    e->savefd[i])
				fcntl(i, F_SETFD, FD_CLOEXEC);
		}
		e->savefd = NULL;
	}
	return (0);
}

#if HAVE_MKNOD
int
c_mknod(const char **wp)
{
	int argc, optc, rv = 0;
	bool ismkfifo = false;
	const char **argv;
	void *set = NULL;
	mode_t mode = 0, oldmode = 0;

	while ((optc = ksh_getopt(wp, &builtin_opt, "m:")) != -1) {
		switch (optc) {
		case 'm':
			set = setmode(builtin_opt.optarg);
			if (set == NULL) {
				bi_errorf("invalid file mode");
				return (1);
			}
			mode = getmode(set, (mode_t)(DEFFILEMODE));
			free_ossetmode(set);
			break;
		default:
			goto c_mknod_usage;
		}
	}
	argv = &wp[builtin_opt.optind];
	if (argv[0] == NULL)
		goto c_mknod_usage;
	for (argc = 0; argv[argc]; argc++)
		;
	if (argc == 2 && argv[1][0] == 'p')
		ismkfifo = true;
	else if (argc != 4 || (argv[1][0] != 'b' && argv[1][0] != 'c'))
		goto c_mknod_usage;

	if (set != NULL)
		oldmode = umask((mode_t)0);
	else
		mode = DEFFILEMODE;

	mode |= (argv[1][0] == 'b') ? S_IFBLK :
	    (argv[1][0] == 'c') ? S_IFCHR : 0;

	if (!ismkfifo) {
		unsigned long majnum, minnum;
		dev_t dv;
		char *c;

		majnum = strtoul(argv[2], &c, 0);
		if ((c == argv[2]) || (*c != '\0')) {
			bi_errorf("non-numeric %s %s '%s'", "device", "major", argv[2]);
			goto c_mknod_err;
		}
		minnum = strtoul(argv[3], &c, 0);
		if ((c == argv[3]) || (*c != '\0')) {
			bi_errorf("non-numeric %s %s '%s'", "device", "minor", argv[3]);
			goto c_mknod_err;
		}
		dv = makedev(majnum, minnum);
		if ((unsigned long)(major(dv)) != majnum) {
			bi_errorf("%s %s too large: %lu", "device", "major", majnum);
			goto c_mknod_err;
		}
		if ((unsigned long)(minor(dv)) != minnum) {
			bi_errorf("%s %s too large: %lu", "device", "minor", minnum);
			goto c_mknod_err;
		}
		if (mknod(argv[0], mode, dv))
			goto c_mknod_failed;
	} else if (mkfifo(argv[0], mode)) {
 c_mknod_failed:
		bi_errorf("%s: %s", argv[0], strerror(errno));
 c_mknod_err:
		rv = 1;
	}

	if (set)
		umask(oldmode);
	return (rv);
 c_mknod_usage:
	bi_errorf("%s: %s", "usage", "mknod [-m mode] name b|c major minor");
	bi_errorf("%s: %s", "usage", "mknod [-m mode] name p");
	return (1);
}
#endif

/*-
   test(1) accepts the following grammar:
	oexpr	::= aexpr | aexpr "-o" oexpr ;
	aexpr	::= nexpr | nexpr "-a" aexpr ;
	nexpr	::= primary | "!" nexpr ;
	primary	::= unary-operator operand
		| operand binary-operator operand
		| operand
		| "(" oexpr ")"
		;

	unary-operator ::= "-a"|"-r"|"-w"|"-x"|"-e"|"-f"|"-d"|"-c"|"-b"|"-p"|
			   "-u"|"-g"|"-k"|"-s"|"-t"|"-z"|"-n"|"-o"|"-O"|"-G"|
			   "-L"|"-h"|"-S"|"-H";

	binary-operator ::= "="|"=="|"!="|"-eq"|"-ne"|"-ge"|"-gt"|"-le"|"-lt"|
			    "-nt"|"-ot"|"-ef"|
			    "<"|">"	# rules used for [[ .. ]] expressions
			    ;
	operand ::= <any thing>
*/

/* POSIX says > 1 for errors */
#define T_ERR_EXIT	2

int
c_test(const char **wp)
{
	int argc, res;
	Test_env te;

	te.flags = 0;
	te.isa = ptest_isa;
	te.getopnd = ptest_getopnd;
	te.eval = test_eval;
	te.error = ptest_error;

	for (argc = 0; wp[argc]; argc++)
		;

	if (strcmp(wp[0], "[") == 0) {
		if (strcmp(wp[--argc], "]") != 0) {
			bi_errorf("missing ]");
			return (T_ERR_EXIT);
		}
	}

	te.pos.wp = wp + 1;
	te.wp_end = wp + argc;

	/*
	 * Handle the special cases from POSIX.2, section 4.62.4.
	 * Implementation of all the rules isn't necessary since
	 * our parser does the right thing for the omitted steps.
	 */
	if (argc <= 5) {
		const char **owp = wp, **owpend = te.wp_end;
		int invert = 0;
		Test_op op;
		const char *opnd1, *opnd2;

		if (argc >= 2 && ((*te.isa)(&te, TM_OPAREN))) {
			te.pos.wp = te.wp_end - 1;
			if ((*te.isa)(&te, TM_CPAREN)) {
				argc -= 2;
				te.wp_end--;
				te.pos.wp = owp + 2;
			} else {
				te.pos.wp = owp + 1;
				te.wp_end = owpend;
			}
		}

		while (--argc >= 0) {
			if ((*te.isa)(&te, TM_END))
				return (!0);
			if (argc == 3) {
				opnd1 = (*te.getopnd)(&te, TO_NONOP, 1);
				if ((op = (*te.isa)(&te, TM_BINOP))) {
					opnd2 = (*te.getopnd)(&te, op, 1);
					res = (*te.eval)(&te, op, opnd1,
					    opnd2, 1);
					if (te.flags & TEF_ERROR)
						return (T_ERR_EXIT);
					if (invert & 1)
						res = !res;
					return (!res);
				}
				/* back up to opnd1 */
				te.pos.wp--;
			}
			if (argc == 1) {
				opnd1 = (*te.getopnd)(&te, TO_NONOP, 1);
				res = (*te.eval)(&te, TO_STNZE, opnd1,
				    NULL, 1);
				if (invert & 1)
					res = !res;
				return (!res);
			}
			if ((*te.isa)(&te, TM_NOT)) {
				invert++;
			} else
				break;
		}
		te.pos.wp = owp + 1;
		te.wp_end = owpend;
	}

	return (test_parse(&te));
}

/*
 * Generic test routines.
 */

Test_op
test_isop(Test_meta meta, const char *s)
{
	char sc1;
	const struct t_op *tbl;

	tbl = meta == TM_UNOP ? u_ops : b_ops;
	if (*s) {
		sc1 = s[1];
		for (; tbl->op_text[0]; tbl++)
			if (sc1 == tbl->op_text[1] && !strcmp(s, tbl->op_text))
				return (tbl->op_num);
	}
	return (TO_NONOP);
}

int
test_eval(Test_env *te, Test_op op, const char *opnd1, const char *opnd2,
    bool do_eval)
{
	int i, s;
	size_t k;
	struct stat b1, b2;
	mksh_ari_t v1, v2;

	if (!do_eval)
		return (0);

	switch (op) {

	/*
	 * Unary Operators
	 */

	/* -n */
	case TO_STNZE:
		return (*opnd1 != '\0');

	/* -z */
	case TO_STZER:
		return (*opnd1 == '\0');

	/* -o */
	case TO_OPTION:
		if ((i = *opnd1) == '!' || i == '?')
			opnd1++;
		if ((k = option(opnd1)) == (size_t)-1)
			return (0);
		return (i == '?' ? 1 : i == '!' ? !Flag(k) : Flag(k));

	/* -r */
	case TO_FILRD:
		/* LINTED use of access */
		return (access(opnd1, R_OK) == 0);

	/* -w */
	case TO_FILWR:
		/* LINTED use of access */
		return (access(opnd1, W_OK) == 0);

	/* -x */
	case TO_FILEX:
		return (ksh_access(opnd1, X_OK) == 0);

	/* -a */
	case TO_FILAXST:
	/* -e */
	case TO_FILEXST:
		return (stat(opnd1, &b1) == 0);

	/* -r */
	case TO_FILREG:
		return (stat(opnd1, &b1) == 0 && S_ISREG(b1.st_mode));

	/* -d */
	case TO_FILID:
		return (stat(opnd1, &b1) == 0 && S_ISDIR(b1.st_mode));

	/* -c */
	case TO_FILCDEV:
		return (stat(opnd1, &b1) == 0 && S_ISCHR(b1.st_mode));

	/* -b */
	case TO_FILBDEV:
		return (stat(opnd1, &b1) == 0 && S_ISBLK(b1.st_mode));

	/* -p */
	case TO_FILFIFO:
		return (stat(opnd1, &b1) == 0 && S_ISFIFO(b1.st_mode));

	/* -h or -L */
	case TO_FILSYM:
		return (lstat(opnd1, &b1) == 0 && S_ISLNK(b1.st_mode));

	/* -S */
	case TO_FILSOCK:
		return (stat(opnd1, &b1) == 0 && S_ISSOCK(b1.st_mode));

	/* -H => HP context dependent files (directories) */
	case TO_FILCDF:
#ifdef S_ISCDF
	{
		char *nv;

		/*
		 * Append a + to filename and check to see if result is
		 * a setuid directory. CDF stuff in general is hookey,
		 * since it breaks for, e.g., the following sequence:
		 * echo hi >foo+; mkdir foo; echo bye >foo/default;
		 * chmod u+s foo (foo+ refers to the file with hi in it,
		 * there is no way to get at the file with bye in it;
		 * please correct me if I'm wrong about this).
		 */

		nv = shf_smprintf("%s+", opnd1);
		i = (stat(nv, &b1) == 0 && S_ISCDF(b1.st_mode));
		afree(nv, ATEMP);
		return (i);
	}
#else
		return (0);
#endif

	/* -u */
	case TO_FILSETU:
		return (stat(opnd1, &b1) == 0 &&
		    (b1.st_mode & S_ISUID) == S_ISUID);

	/* -g */
	case TO_FILSETG:
		return (stat(opnd1, &b1) == 0 &&
		    (b1.st_mode & S_ISGID) == S_ISGID);

	/* -k */
	case TO_FILSTCK:
#ifdef S_ISVTX
		return (stat(opnd1, &b1) == 0 &&
		    (b1.st_mode & S_ISVTX) == S_ISVTX);
#else
		return (0);
#endif

	/* -s */
	case TO_FILGZ:
		return (stat(opnd1, &b1) == 0 && b1.st_size > 0L);

	/* -t */
	case TO_FILTT:
		if (opnd1 && !bi_getn(opnd1, &i)) {
			te->flags |= TEF_ERROR;
			i = 0;
		} else
			i = isatty(opnd1 ? i : 0);
		return (i);

	/* -O */
	case TO_FILUID:
		return (stat(opnd1, &b1) == 0 && b1.st_uid == ksheuid);

	/* -G */
	case TO_FILGID:
		return (stat(opnd1, &b1) == 0 && b1.st_gid == getegid());

	/*
	 * Binary Operators
	 */

	/* = */
	case TO_STEQL:
		if (te->flags & TEF_DBRACKET)
			return (gmatchx(opnd1, opnd2, false));
		return (strcmp(opnd1, opnd2) == 0);

	/* != */
	case TO_STNEQ:
		if (te->flags & TEF_DBRACKET)
			return (!gmatchx(opnd1, opnd2, false));
		return (strcmp(opnd1, opnd2) != 0);

	/* < */
	case TO_STLT:
		return (strcmp(opnd1, opnd2) < 0);

	/* > */
	case TO_STGT:
		return (strcmp(opnd1, opnd2) > 0);

	/* -eq */
	case TO_INTEQ:
	/* -ne */
	case TO_INTNE:
	/* -ge */
	case TO_INTGE:
	/* -gt */
	case TO_INTGT:
	/* -le */
	case TO_INTLE:
	/* -lt */
	case TO_INTLT:
		if (!evaluate(opnd1, &v1, KSH_RETURN_ERROR, false) ||
		    !evaluate(opnd2, &v2, KSH_RETURN_ERROR, false)) {
			/* error already printed.. */
			te->flags |= TEF_ERROR;
			return (1);
		}
		switch (op) {
		case TO_INTEQ:
			return (v1 == v2);
		case TO_INTNE:
			return (v1 != v2);
		case TO_INTGE:
			return (v1 >= v2);
		case TO_INTGT:
			return (v1 > v2);
		case TO_INTLE:
			return (v1 <= v2);
		case TO_INTLT:
			return (v1 < v2);
		default:
			/* NOTREACHED */
			break;
		}
		/* NOTREACHED */

	/* -nt */
	case TO_FILNT:
		/*
		 * ksh88/ksh93 succeed if file2 can't be stated
		 * (subtly different from 'does not exist').
		 */
		return (stat(opnd1, &b1) == 0 &&
		    (((s = stat(opnd2, &b2)) == 0 &&
		    b1.st_mtime > b2.st_mtime) || s < 0));

	/* -ot */
	case TO_FILOT:
		/*
		 * ksh88/ksh93 succeed if file1 can't be stated
		 * (subtly different from 'does not exist').
		 */
		return (stat(opnd2, &b2) == 0 &&
		    (((s = stat(opnd1, &b1)) == 0 &&
		    b1.st_mtime < b2.st_mtime) || s < 0));

	/* -ef */
	case TO_FILEQ:
		return (stat (opnd1, &b1) == 0 && stat (opnd2, &b2) == 0 &&
		    b1.st_dev == b2.st_dev && b1.st_ino == b2.st_ino);

	/* all other cases */
	case TO_NONOP:
	case TO_NONNULL:
		/* throw the error */
		break;
	}
	(*te->error)(te, 0, "internal error: unknown op");
	return (1);
}

int
test_parse(Test_env *te)
{
	int rv;

	rv = test_oexpr(te, 1);

	if (!(te->flags & TEF_ERROR) && !(*te->isa)(te, TM_END))
		(*te->error)(te, 0, "unexpected operator/operand");

	return ((te->flags & TEF_ERROR) ? T_ERR_EXIT : !rv);
}

static int
test_oexpr(Test_env *te, bool do_eval)
{
	int rv;

	if ((rv = test_aexpr(te, do_eval)))
		do_eval = false;
	if (!(te->flags & TEF_ERROR) && (*te->isa)(te, TM_OR))
		return (test_oexpr(te, do_eval) || rv);
	return (rv);
}

static int
test_aexpr(Test_env *te, bool do_eval)
{
	int rv;

	if (!(rv = test_nexpr(te, do_eval)))
		do_eval = false;
	if (!(te->flags & TEF_ERROR) && (*te->isa)(te, TM_AND))
		return (test_aexpr(te, do_eval) && rv);
	return (rv);
}

static int
test_nexpr(Test_env *te, bool do_eval)
{
	if (!(te->flags & TEF_ERROR) && (*te->isa)(te, TM_NOT))
		return (!test_nexpr(te, do_eval));
	return (test_primary(te, do_eval));
}

static int
test_primary(Test_env *te, bool do_eval)
{
	const char *opnd1, *opnd2;
	int rv;
	Test_op op;

	if (te->flags & TEF_ERROR)
		return (0);
	if ((*te->isa)(te, TM_OPAREN)) {
		rv = test_oexpr(te, do_eval);
		if (te->flags & TEF_ERROR)
			return (0);
		if (!(*te->isa)(te, TM_CPAREN)) {
			(*te->error)(te, 0, "missing )");
			return (0);
		}
		return (rv);
	}
	/*
	 * Binary should have precedence over unary in this case
	 * so that something like test \( -f = -f \) is accepted
	 */
	if ((te->flags & TEF_DBRACKET) || (&te->pos.wp[1] < te->wp_end &&
	    !test_isop(TM_BINOP, te->pos.wp[1]))) {
		if ((op = (*te->isa)(te, TM_UNOP))) {
			/* unary expression */
			opnd1 = (*te->getopnd)(te, op, do_eval);
			if (!opnd1) {
				(*te->error)(te, -1, "missing argument");
				return (0);
			}

			return ((*te->eval)(te, op, opnd1, NULL, do_eval));
		}
	}
	opnd1 = (*te->getopnd)(te, TO_NONOP, do_eval);
	if (!opnd1) {
		(*te->error)(te, 0, "expression expected");
		return (0);
	}
	if ((op = (*te->isa)(te, TM_BINOP))) {
		/* binary expression */
		opnd2 = (*te->getopnd)(te, op, do_eval);
		if (!opnd2) {
			(*te->error)(te, -1, "missing second argument");
			return (0);
		}

		return ((*te->eval)(te, op, opnd1, opnd2, do_eval));
	}
	return ((*te->eval)(te, TO_STNZE, opnd1, NULL, do_eval));
}

/*
 * Plain test (test and [ .. ]) specific routines.
 */

/*
 * Test if the current token is a whatever. Accepts the current token if
 * it is. Returns 0 if it is not, non-zero if it is (in the case of
 * TM_UNOP and TM_BINOP, the returned value is a Test_op).
 */
static Test_op
ptest_isa(Test_env *te, Test_meta meta)
{
	/* Order important - indexed by Test_meta values */
	static const char *const tokens[] = {
		"-o", "-a", "!", "(", ")"
	};
	Test_op rv;

	if (te->pos.wp >= te->wp_end)
		return (meta == TM_END ? TO_NONNULL : TO_NONOP);

	if (meta == TM_UNOP || meta == TM_BINOP)
		rv = test_isop(meta, *te->pos.wp);
	else if (meta == TM_END)
		rv = TO_NONOP;
	else
		rv = !strcmp(*te->pos.wp, tokens[(int)meta]) ?
		    TO_NONNULL : TO_NONOP;

	/* Accept the token? */
	if (rv != TO_NONOP)
		te->pos.wp++;

	return (rv);
}

static const char *
ptest_getopnd(Test_env *te, Test_op op, bool do_eval MKSH_A_UNUSED)
{
	if (te->pos.wp >= te->wp_end)
		return (op == TO_FILTT ? "1" : NULL);
	return (*te->pos.wp++);
}

static void
ptest_error(Test_env *te, int ofs, const char *msg)
{
	const char *op;

	te->flags |= TEF_ERROR;
	if ((op = te->pos.wp + ofs >= te->wp_end ? NULL : te->pos.wp[ofs]))
		bi_errorf("%s: %s", op, msg);
	else
		bi_errorf("%s", msg);
}

#ifndef MKSH_NO_LIMITS
#define SOFT	0x1
#define HARD	0x2

struct limits {
	const char *name;
	int resource;		/* resource to get/set */
	int factor;		/* multiply by to get rlim_{cur,max} values */
	char option;
};

static void print_ulimit(const struct limits *, int);
static int set_ulimit(const struct limits *, const char *, int);

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
#if defined(ULIMIT_M_IS_RSS) && defined(RLIMIT_AS) && (RLIMIT_RSS == RLIMIT_AS)
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


int
c_ulimit(const char **wp)
{
	static const struct limits limits[] = {
		/* do not use options -H, -S or -a or change the order */
#ifdef RLIMIT_CPU
		{ "time(cpu-seconds)", RLIMIT_CPU, 1, 't' },
#endif
#ifdef RLIMIT_FSIZE
		{ "file(blocks)", RLIMIT_FSIZE, 512, 'f' },
#endif
#ifdef RLIMIT_CORE
		{ "coredump(blocks)", RLIMIT_CORE, 512, 'c' },
#endif
#ifdef RLIMIT_DATA
		{ "data(KiB)", RLIMIT_DATA, 1024, 'd' },
#endif
#ifdef RLIMIT_STACK
		{ "stack(KiB)", RLIMIT_STACK, 1024, 's' },
#endif
#ifdef RLIMIT_MEMLOCK
		{ "lockedmem(KiB)", RLIMIT_MEMLOCK, 1024, 'l' },
#endif
#ifdef RLIMIT_NOFILE
		{ "nofiles(descriptors)", RLIMIT_NOFILE, 1, 'n' },
#endif
#ifdef RLIMIT_NPROC
		{ "processes", RLIMIT_NPROC, 1, 'p' },
#endif
#ifdef RLIMIT_SWAP
		{ "swap(KiB)", RLIMIT_SWAP, 1024, 'w' },
#endif
#ifdef RLIMIT_LOCKS
		{ "flocks", RLIMIT_LOCKS, -1, 'L' },
#endif
#ifdef RLIMIT_TIME
		{ "humantime(seconds)", RLIMIT_TIME, 1, 'T' },
#endif
#ifdef RLIMIT_NOVMON
		{ "vnodemonitors", RLIMIT_NOVMON, 1, 'V' },
#endif
#ifdef RLIMIT_SIGPENDING
		{ "sigpending", RLIMIT_SIGPENDING, 1, 'i' },
#endif
#ifdef RLIMIT_MSGQUEUE
		{ "msgqueue(bytes)", RLIMIT_MSGQUEUE, 1, 'q' },
#endif
#ifdef RLIMIT_AIO_MEM
		{ "AIOlockedmem(KiB)", RLIMIT_AIO_MEM, 1024, 'M' },
#endif
#ifdef RLIMIT_AIO_OPS
		{ "AIOoperations", RLIMIT_AIO_OPS, 1, 'O' },
#endif
#ifdef RLIMIT_TCACHE
		{ "cachedthreads", RLIMIT_TCACHE, 1, 'C' },
#endif
#ifdef RLIMIT_SBSIZE
		{ "sockbufsiz(KiB)", RLIMIT_SBSIZE, 1024, 'B' },
#endif
#ifdef RLIMIT_PTHREAD
		{ "threadsperprocess", RLIMIT_PTHREAD, 1, 'P' },
#endif
#ifdef RLIMIT_NICE
		{ "maxnice", RLIMIT_NICE, 1, 'e' },
#endif
#ifdef RLIMIT_RTPRIO
		{ "maxrtprio", RLIMIT_RTPRIO, 1, 'r' },
#endif
#if defined(ULIMIT_M_IS_RSS)
		{ "resident-set(KiB)", RLIMIT_RSS, 1024, 'm' },
#elif defined(ULIMIT_M_IS_VMEM)
		{ "memory(KiB)", RLIMIT_VMEM, 1024, 'm' },
#endif
#if defined(ULIMIT_V_IS_VMEM)
		{ "virtual-memory(KiB)", RLIMIT_VMEM, 1024, 'v' },
#elif defined(ULIMIT_V_IS_AS)
		{ "address-space(KiB)", RLIMIT_AS, 1024, 'v' },
#endif
		{ NULL, 0, 0, 0 }
	};
	static char opts[3 + NELEM(limits)];
	int how = SOFT | HARD, optc, what = 'f';
	bool all = false;
	const struct limits *l;

	if (!opts[0]) {
		/* build options string on first call - yuck */
		char *p = opts;

		*p++ = 'H'; *p++ = 'S'; *p++ = 'a';
		for (l = limits; l->name; l++)
			*p++ = l->option;
		*p = '\0';
	}

	while ((optc = ksh_getopt(wp, &builtin_opt, opts)) != -1)
		switch (optc) {
		case 'H':
			how = HARD;
			break;
		case 'S':
			how = SOFT;
			break;
		case 'a':
			all = true;
			break;
		case '?':
			bi_errorf("%s: %s", "usage",
			    "ulimit [-acdfHLlmnpSsTtvw] [value]");
			return (1);
		default:
			what = optc;
		}

	for (l = limits; l->name && l->option != what; l++)
		;
	if (!l->name) {
		internal_warningf("ulimit: %c", what);
		return (1);
	}

	if (wp[builtin_opt.optind]) {
		if (all || wp[builtin_opt.optind + 1]) {
			bi_errorf("too many arguments");
			return (1);
		}
		return (set_ulimit(l, wp[builtin_opt.optind], how));
	}
	if (!all)
		print_ulimit(l, how);
	else for (l = limits; l->name; l++) {
		shprintf("%-20s ", l->name);
		print_ulimit(l, how);
	}
	return (0);
}

static int
set_ulimit(const struct limits *l, const char *v, int how)
{
	rlim_t val = (rlim_t)0;
	struct rlimit limit;

	if (strcmp(v, "unlimited") == 0)
		val = (rlim_t)RLIM_INFINITY;
	else {
		mksh_ari_t rval;

		if (!evaluate(v, &rval, KSH_RETURN_ERROR, false))
			return (1);
		/*
		 * Avoid problems caused by typos that evaluate misses due
		 * to evaluating unset parameters to 0...
		 * If this causes problems, will have to add parameter to
		 * evaluate() to control if unset params are 0 or an error.
		 */
		if (!rval && !ksh_isdigit(v[0])) {
			bi_errorf("invalid %s limit: %s", l->name, v);
			return (1);
		}
		val = (rlim_t)((rlim_t)rval * l->factor);
	}

	if (getrlimit(l->resource, &limit) < 0) {
		/* some can't be read, e.g. Linux RLIMIT_LOCKS */
		limit.rlim_cur = RLIM_INFINITY;
		limit.rlim_max = RLIM_INFINITY;
	}
	if (how & SOFT)
		limit.rlim_cur = val;
	if (how & HARD)
		limit.rlim_max = val;
	if (!setrlimit(l->resource, &limit))
		return (0);
	if (errno == EPERM)
		bi_errorf("%s exceeds allowable %s limit", v, l->name);
	else
		bi_errorf("bad %s limit: %s", l->name, strerror(errno));
	return (1);
}

static void
print_ulimit(const struct limits *l, int how)
{
	rlim_t val = (rlim_t)0;
	struct rlimit limit;

	if (getrlimit(l->resource, &limit)) {
		shf_puts("unknown\n", shl_stdout);
		return;
	}
	if (how & SOFT)
		val = limit.rlim_cur;
	else if (how & HARD)
		val = limit.rlim_max;
	if (val == (rlim_t)RLIM_INFINITY)
		shf_puts("unlimited\n", shl_stdout);
	else
		shprintf("%ld\n", (long)(val / l->factor));
}
#endif

int
c_rename(const char **wp)
{
	int rv = 1;

	/* skip argv[0] */
	++wp;
	if (wp[0] && !strcmp(wp[0], "--"))
		/* skip "--" (options separator) */
		++wp;

	/* check for exactly two arguments */
	if (wp[0] == NULL	/* first argument */ ||
	    wp[1] == NULL	/* second argument */ ||
	    wp[2] != NULL	/* no further args please */)
		bi_errorf(Tsynerr);
	else if ((rv = rename(wp[0], wp[1])) != 0) {
		rv = errno;
		bi_errorf("%s: %s", "failed", strerror(rv));
	}

	return (rv);
}

int
c_realpath(const char **wp)
{
	int rv = 1;
	char *buf;

	/* skip argv[0] */
	++wp;
	if (wp[0] && !strcmp(wp[0], "--"))
		/* skip "--" (options separator) */
		++wp;

	/* check for exactly one argument */
	if (wp[0] == NULL || wp[1] != NULL)
		bi_errorf(Tsynerr);
	else if ((buf = do_realpath(wp[0])) == NULL) {
		rv = errno;
		bi_errorf("%s: %s", wp[0], strerror(rv));
		if ((unsigned int)rv > 255)
			rv = 255;
	} else {
		shprintf("%s\n", buf);
		afree(buf, ATEMP);
		rv = 0;
	}

	return (rv);
}

int
c_cat(const char **wp)
{
	int fd = STDIN_FILENO, rv;
	ssize_t n, w;
	const char *fn = "<stdin>";
	char *buf, *cp;
#define MKSH_CAT_BUFSIZ 4096

	if ((buf = malloc_osfunc(MKSH_CAT_BUFSIZ)) == NULL) {
		bi_errorf(Toomem, (unsigned long)MKSH_CAT_BUFSIZ);
		return (1);
	}

	/* parse options: POSIX demands we support "-u" as no-op */
	while ((rv = ksh_getopt(wp, &builtin_opt, "u")) != -1) {
		switch (rv) {
		case 'u':
			/* we already operate unbuffered */
			break;
		default:
			bi_errorf(Tsynerr);
			return (1);
		}
	}
	wp += builtin_opt.optind;
	rv = 0;

	do {
		if (*wp) {
			fn = *wp++;
			if (fn[0] == '-' && fn[1] == '\0')
				fd = STDIN_FILENO;
			else if ((fd = open(fn, O_RDONLY)) < 0) {
				rv = errno;
				bi_errorf("%s: %s", fn, strerror(rv));
				rv = 1;
				continue;
			}
		}
		while (/* CONSTCOND */ 1) {
			n = blocking_read(fd, (cp = buf), MKSH_CAT_BUFSIZ);
			if (n == -1) {
				if (errno == EINTR) {
					/* give the user a chance to ^C out */
					intrcheck();
					/* interrupted, try again */
					continue;
				}
				/* an error occured during reading */
				rv = errno;
				bi_errorf("%s: %s", fn, strerror(rv));
				rv = 1;
				break;
			} else if (n == 0)
				/* end of file reached */
				break;
			while (n) {
				w = write(STDOUT_FILENO, cp, n);
				if (w == -1) {
					if (errno == EINTR)
						/* interrupted, try again */
						continue;
					/* an error occured during writing */
					rv = errno;
					bi_errorf("%s: %s", "<stdout>",
					    strerror(rv));
					rv = 1;
					if (fd != STDIN_FILENO)
						close(fd);
					goto out;
				}
				n -= w;
				cp += w;
			}
		}
		if (fd != STDIN_FILENO)
			close(fd);
	} while (*wp);

 out:
	free_osfunc(buf);
	return (rv);
}

#if HAVE_SELECT
int
c_sleep(const char **wp)
{
	struct timeval tv;
	int rv = 1;

	/* skip argv[0] */
	++wp;
	if (wp[0] && !strcmp(wp[0], "--"))
		/* skip "--" (options separator) */
		++wp;

	if (!wp[0] || wp[1])
		bi_errorf(Tsynerr);
	else if (parse_usec(wp[0], &tv))
		bi_errorf("%s: %s '%s'", Tsynerr, strerror(errno), wp[0]);
	else {
#ifndef MKSH_NOPROSPECTOFWORK
		sigset_t omask;

		/* block SIGCHLD from interrupting us, though */
		sigprocmask(SIG_BLOCK, &sm_sigchld, &omask);
#endif
		if (select(0, NULL, NULL, NULL, &tv) == 0 || errno == EINTR)
			/*
			 * strictly speaking only for SIGALRM, but the
			 * execution may be interrupted by other signals
			 */
			rv = 0;
		else
			bi_errorf("%s: %s", Tselect, strerror(errno));
#ifndef MKSH_NOPROSPECTOFWORK
		sigprocmask(SIG_SETMASK, &omask, NULL);
#endif
	}
	return (rv);
}
#endif

#if defined(ANDROID)
static int
c_android_lsmod(const char **wp MKSH_A_UNUSED)
{
	const char *cwp[3] = { "cat", "/proc/modules", NULL };

	builtin_argv0 = cwp[0];
	return (c_cat(cwp));
}
#endif
