/*	$OpenBSD: main.c,v 1.57 2015/09/10 22:48:58 nicm Exp $	*/
/*	$OpenBSD: tty.c,v 1.10 2014/08/10 02:44:26 guenther Exp $	*/
/*	$OpenBSD: io.c,v 1.26 2015/09/11 08:00:27 guenther Exp $	*/
/*	$OpenBSD: table.c,v 1.16 2015/09/01 13:12:31 tedu Exp $	*/

/*-
 * Copyright (c) 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010,
 *		 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018,
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

#define EXTERN
#include "sh.h"

__RCSID("$MirOS: src/bin/mksh/main.c,v 1.436 2023/09/17 00:44:34 tg Exp $");
__IDSTRING(mbsdcc_h_rcsid, SYSKERN_MBSDCC_H);
__IDSTRING(mbsdint_h_rcsid, SYSKERN_MBSDINT_H);
__IDSTRING(sh_h_rcsid, MKSH_SH_H_ID);

#ifndef MKSHRC_PATH
#define MKSHRC_PATH	"~/.mkshrc"
#endif

#ifndef MKSH_DEFAULT_TMPDIR
#define MKSH_DEFAULT_TMPDIR	MKSH_UNIXROOT "/tmp"
#endif

#if !HAVE_POSIX_UTF8_LOCALE
/* this is the “implementation-defined default locale” */
#ifdef MKSH_DEFAULT_UTFLOC
#define MKSH_DEFAULT_LOCALE	"UTF-8"
#else
#define MKSH_DEFAULT_LOCALE	"C"
#endif
#endif

static void main_init(int, const char *[], Source **);
void chvt_reinit(void);
static void reclaim(void);
static void remove_temps(struct temp *);
static void rndsetup(void);
static void init_environ(void);
#ifdef SIGWINCH
static void x_sigwinch(int);
#endif

#ifdef DEBUG
static void reclim_trace(void);
#else
#define reclim_trace() /* nothing */
#endif

static const char initsubs[] =
    "${PS2=> }"
    "${PS3=#? }"
    "${PS4=+ }"
    "${SECONDS=0}"
    "${TMOUT=0}"
    "${EPOCHREALTIME=}";

static const char *initcoms[] = {
	Ttypeset, Tdr, initvsn, NULL,
	Ttypeset, Tdx, "HOME", TPATH, TSHELL, NULL,
	Ttypeset, "-i10", "COLUMNS", "LINES", "SECONDS", "TMOUT", NULL,
	Talias,
	"integer=\\\\builtin typeset -i",
	"local=\\\\builtin typeset",
	/* not "alias -t --": hash -r needs to work */
	"hash=\\\\builtin alias -t",
	"type=\\\\builtin whence -v",
	"autoload=\\\\builtin typeset -fu",
	"functions=\\\\builtin typeset -f",
	"history=\\\\builtin fc -l",
	"nameref=\\\\builtin typeset -n",
	"nohup=nohup ",
	"r=\\\\builtin fc -e -",
	"login=\\\\builtin exec login",
	NULL,
	 /* this is what AT&T ksh seems to track, with the addition of emacs */
	Talias, "-tU",
	"cat", "cc", "chmod", "cp", "date", "ed", "emacs", "grep", "ls",
	"make", "mv", "pr", "rm", "sed", Tsh, "vi", "who", NULL,
	NULL
};

static const char *restr_com[] = {
	Ttypeset, Tdr, TENV, "HISTFILE", TPATH, TSHELL, NULL
};

extern const char Tpipest[];

/* top-level parsing and execution environment */
static struct env env;
struct env *e = &env;

/* buffer */
#if !HAVE_GET_CURRENT_DIR_NAME
static size_t getwd_bufsz = 448U;
#endif
static char *getwd_bufp = NULL;

/* many compile-time assertions */
mbCTA_BEG(main_c);

 /* require char to be 8 bit long */
 mbCTA(char_8bit, (CHAR_BIT) == 8 &&
    (((unsigned int)(unsigned char)255U) == 255U) &&
    (((unsigned int)(unsigned char)256U) == 0U) &&
    mbiTYPE_UBITS(unsigned char) == 8U &&
    mbiMASK_BITS(SCHAR_MAX) == 7U);

 /* the next assertion is probably not really needed */
 mbCTA(short_is_2_char, sizeof(short) == 2);
 /* the next assertion is probably not really needed */
 mbCTA(int_is_4_char, sizeof(int) == 4);

#ifndef MKSH_LEGACY_MODE
 mbiCTA_TYPE_MBIT(sari, mksh_ari_t);
 mbiCTA_TYPE_MBIT(uari, mksh_uari_t);
 mbCTA(basic_int32_smask, mbiMASK_CHK(INT32_MAX));
 mbCTA(basic_int32_umask, mbiMASK_CHK(UINT32_MAX));
 mbCTA(basic_int32_ari,
    mbiTYPE_UMAX(mksh_uari_t) == (UINT32_MAX) &&
    /* require two’s complement */
    ((INT32_MIN)+1 == -(INT32_MAX)));
 /* the next assertion is probably not really needed */
 mbCTA(ari_is_4_char, sizeof(mksh_ari_t) == 4);
 /* but this is */
 mbCTA(ari_has_31_bit, mbiMASK_BITS(INT32_MAX) == 31);
 /* the next assertion is probably not really needed */
 mbCTA(uari_is_4_char, sizeof(mksh_uari_t) == 4);
 mbCTA(uari_is_32_bit, mbiTYPE_UBITS(mksh_uari_t) == 32);
#else
 mbCTA(long_complement, (LONG_MIN)+1 == -(LONG_MAX));
#endif
 /* these are always required */
 mbCTA(ari_is_signed, !mbiTYPE_ISU(mksh_ari_t));
 mbCTA(uari_is_unsigned, mbiTYPE_ISU(mksh_uari_t));
 /* we require these to have the precisely same size and assume 2s complement */
 mbCTA(ari_size_no_matter_of_signedness,
    sizeof(mksh_ari_t) == sizeof(mksh_uari_t));

 /* our formatting routines assume this */
 mbCTA(ari_fits_in_long, sizeof(mksh_ari_t) <= sizeof(long));

mbCTA_END(main_c);
/* end of compile-time asserts */

static void
rndsetup(void)
{
	struct {
		ALLOC_ITEM alloc_INT;
		void *bssptr, *dataptr, *stkptr, *mallocptr;
		kshjmp_buf jbuf;
		struct timeval tv;
	} *bufptr;
	char *cp;

	cp = alloc(sizeof(*bufptr) - sizeof(ALLOC_ITEM), APERM);
	/* clear the allocated space, for valgrind and to avoid UB */
	memset(cp, 0, sizeof(*bufptr) - sizeof(ALLOC_ITEM));
	/* undo what alloc() did to the malloc result address */
	bufptr = (void *)(cp - sizeof(ALLOC_ITEM));
	/* PIE or something similar provides us with deltas here */
	bufptr->bssptr = &rndsetupstate;
	bufptr->dataptr = &e;
	/* ASLR in at least Windows, Linux, some BSDs */
	bufptr->stkptr = &bufptr;
	/* randomised malloc in BSD (and possibly others) */
	bufptr->mallocptr = bufptr;
	/* glibc pointer guard */
#ifdef MKSH_NO_SIGSETJMP
	setjmp(bufptr->jbuf);
#else
	sigsetjmp(bufptr->jbuf, 1);
#endif
	/* introduce variation (cannot use gettimeofday *tzp portably) */
	mksh_TIME(bufptr->tv);

#ifdef MKSH_ALLOC_CATCH_UNDERRUNS
	mprotect(((char *)bufptr) + 4096, 4096, PROT_READ | PROT_WRITE);
#endif
	chvt_rndsetup(bufptr, sizeof(*bufptr));
	afree(cp, APERM);
}

/* pre-initio() */
void
chvt_reinit(void)
{
	kshpid = procpid = getpid();
	ksheuid = geteuid();
	kshpgrp = getpgrp();
	kshppid = getppid();
}

static const char *empty_argv[] = {
	Tmksh, NULL
};

#ifndef MKSH_EARLY_LOCALE_TRACKING
static kby
isuc(const char *cx) {
	const char *cp;

	if (!cx || !*cx)
		return (0);

	if ((cp = cstrchr(cx, '.')))
		++cp;
	else
		cp = cx;
	if (!isCh(cp[0], 'U', 'u') ||
	    !isCh(cp[1], 'T', 't') ||
	    !isCh(cp[2], 'F', 'f'))
		return (0);
	cp += isch(cp[3], '-') ? 4 : 3;
	return (isch(*cp, '8') && (isch(cp[1], '@') || !cp[1]));
}
#endif

kby
kshname_islogin(const char **kshbasenamep)
{
	const char *cp;
	size_t o;
	kby rv;

	/* determine the basename (without '-' or path) of the executable */
	cp = kshname;
	o = 0;
	while ((rv = cp[o++])) {
		if (mksh_cdirsep(rv)) {
			cp += o;
			o = 0;
		}
	}
	rv = isch(*cp, '-') || isch(*kshname, '-');
	if (isch(*cp, '-'))
		++cp;
	if (!*cp)
		cp = empty_argv[0];
	*kshbasenamep = cp;
	return (rv);
}

/* pre-initio() */
static void
main_init(int argc, const char *argv[], Source **sp)
{
	int argi = 1, i;
	Source *s = NULL;
	unsigned char restricted_shell = 0, errexit, utf_flag;
	char *cp;
	const char *ccp, **wp;
	struct tbl *vp;
	struct stat s_stdin;
#if !defined(_PATH_DEFPATH) && defined(_CS_PATH)
	ssize_t k;
#endif

#if defined(MKSH_EBCDIC) || defined(MKSH_FAUX_EBCDIC)
	/* this must come *really* early due to locale use */
	ebcdic_init();
#endif
	set_ifs(TC_IFSWS);

#ifdef __OS2__
	os2_init(&argc, &argv);
#define builtin_name_cmp stricmp
#else
#define builtin_name_cmp strcmp
#endif

	/* do things like getpgrp() et al. */
	chvt_reinit();

	/* make sure argv[] is sane, for weird OSes */
	if (!*argv) {
		argv = empty_argv;
		argc = 1;
	}
	kshname = argv[0];

	/* initialise permanent Area */
	ainit(&aperm);
	/* max. name length: -2147483648 = 11 (+ NUL) */
	vtemp = alloc(offsetof(struct tbl, name[0]) + 12U, APERM);
#if !HAVE_GET_CURRENT_DIR_NAME
	getwd_bufp = alloc(getwd_bufsz + 1U, APERM);
	getwd_bufp[getwd_bufsz] = '\0';
#endif

	/* set up base environment */
	env.type = E_NONE;
	ainit(&env.area);
	/* set up global l->vars and l->funs */
	newblock();

	/* Do this first so output routines (eg. kwarnf, shellf) can work */
	initio();

	/* check kshname: leading dash, determine basename */
	Flag(FLOGIN) = kshname_islogin(&ccp);

	/*
	 * Turn on nohup by default. (AT&T ksh does not have a nohup
	 * option - it always sends the hup).
	 */
	Flag(FNOHUP) = 1;

	/*
	 * Turn on brace expansion by default. AT&T kshs that have
	 * alternation always have it on.
	 */
	Flag(FBRACEEXPAND) = 1;

	/*
	 * Turn on "set -x" inheritance by default.
	 */
	Flag(FXTRACEREC) = 1;

	/* define built-in commands and see if we were called as one */
	ktinit(APERM, &builtins,
	    /* currently up to 50 builtins: 75% of 128 = 2^7 */
	    7);
	for (i = 0; mkshbuiltins[i].name != NULL; ++i) {
		const char *builtin_name;

		builtin_name = builtin(mkshbuiltins[i].name,
		    mkshbuiltins[i].func);
		if (!builtin_name_cmp(ccp, builtin_name)) {
			/* canonicalise argv[0] */
			ccp = builtin_name;
			as_builtin = Ja;
		}
	}

	if (!as_builtin) {
		/* check for -T option early */
		argi = parse_args(argv, OF_FIRSTTIME, NULL);
		if (argi < 0)
			unwind(LERROR);
		/* called as rsh, rmksh, -rsh, RKSH.EXE, etc.? */
		if (isCh(*ccp, 'R', 'r')) {
			++ccp;
			++restricted_shell;
		}
#if defined(MKSH_BINSHPOSIX) || defined(MKSH_BINSHREDUCED)
		/* are we called as -rsh or /bin/sh or SH.EXE or so? */
		if (isCh(ccp[0], 'S', 's') &&
		    isCh(ccp[1], 'H', 'h')) {
			/* either also turns off braceexpand */
#ifdef MKSH_BINSHPOSIX
			/* enable better POSIX conformance */
			change_flag(FPOSIX, OF_FIRSTTIME, Ja);
#endif
#ifdef MKSH_BINSHREDUCED
			/* enable kludge/compat mode */
			change_flag(FSH, OF_FIRSTTIME, Ja);
#endif
		}
#endif
	}

	initvar();

	inittraps();

	coproc_init();

	/* set up variable and command dictionaries */
	ktinit(APERM, &taliases, 0);
	ktinit(APERM, &aliases, 0);
#ifndef MKSH_NOPWNAM
	ktinit(APERM, &homedirs, 0);
#endif

	/* define shell keywords */
	initkeywords();

	init_histvec();

	/* initialise tty size before importing environment */
	change_winsz();

#ifdef _PATH_DEFPATH
	def_path = _PATH_DEFPATH;
#else
#ifdef _CS_PATH
	if ((k = confstr(_CS_PATH, NULL, 0)) > 0 &&
	    confstr(_CS_PATH, cp = alloc(k + 1, APERM), k + 1) == k + 1)
		def_path = cp;
	else
#endif
		/*
		 * this is uniform across all OSes unless it
		 * breaks somewhere hard; don't try to optimise,
		 * e.g. add stuff for Interix or remove /usr
		 * for HURD, because e.g. Debian GNU/HURD is
		 * "keeping a regular /usr"; this is supposed
		 * to be a sane 'basic' default PATH
		 */
		def_path = MKSH_UNIXROOT "/bin" MKSH_PATHSEPS
		    MKSH_UNIXROOT "/usr/bin" MKSH_PATHSEPS
		    MKSH_UNIXROOT "/sbin" MKSH_PATHSEPS
		    MKSH_UNIXROOT "/usr/sbin";
#endif

	/*
	 * Set PATH to def_path (will set the path global variable).
	 * Import of environment below will probably change this setting;
	 * the EXPORT flag is only added via initcoms for this to work.
	 */
	vp = global(TPATH);
	/* setstr can't fail here */
	setstr(vp, def_path, KSH_RETURN_ERROR);

#ifndef MKSH_NO_CMDLINE_EDITING
	/*
	 * Set edit mode to emacs by default, may be overridden
	 * by the environment or the user. Also, we want tab completion
	 * on in vi by default.
	 */
	change_flag(FEMACS, OF_INTERNAL, Ja);
#if !MKSH_S_NOVI
	Flag(FVITABCOMPLETE) = 1;
#endif
#endif

	/* import environment */
	init_environ();

	/* for security */
	typeset(TinitIFS, 0, 0, 0, 0);

	/* assign default shell variable values */
	typeset("PATHSEP=" MKSH_PATHSEPS, 0, 0, 0, 0);
	substitute(initsubs, 0);

	/* Figure out the current working directory and set $PWD */
	vp = global(TPWD);
	cp = str_val(vp);
	/* Try to use existing $PWD if it is valid */
	set_current_wd((mksh_abspath(cp) && test_eval(NULL, TO_FILEQ, cp,
	    Tdot, Ja)) ? cp : NULL);
	if (current_wd[0])
		simplify_path(current_wd);
	/* Only set pwd if we know where we are or if it had a bogus value */
	if (current_wd[0] || *cp)
		/* setstr can't fail here */
		setstr(vp, current_wd, KSH_RETURN_ERROR);

	for (wp = initcoms; *wp != NULL; wp++) {
		c_builtin(wp);
		while (*wp != NULL)
			wp++;
	}
	setint_n(global("OPTIND"), 1, 10);

	kshuid = getuid();
	kshgid = getgid();
	kshegid = getegid();
	rndsetup();

	safe_prompt = ksheuid ? "$ " : "# ";
	vp = global("PS1");
	/* Set PS1 if unset or we are root and prompt doesn't contain a # */
	if (!(vp->flag & ISSET) ||
	    (!ksheuid && !vstrchr(str_val(vp), '#')))
		/* setstr can't fail here */
		setstr(vp, safe_prompt, KSH_RETURN_ERROR);
	/* ensure several variables will be (unsigned) integers */
	setint_n((vp = global("BASHPID")), 0, 10);
	vp->flag |= INT_U;
	setint_n((vp = global("PGRP")), (mksh_uari_t)kshpgrp, 10);
	vp->flag |= INT_U;
	setint_n((vp = global("PPID")), (mksh_uari_t)kshppid, 10);
	vp->flag |= INT_U;
	setint_n((vp = global("USER_ID")), (mksh_uari_t)ksheuid, 10);
	vp->flag |= INT_U;
	setint_n((vp = global("KSHUID")), (mksh_uari_t)kshuid, 10);
	vp->flag |= INT_U;
	setint_n((vp = global("KSHEGID")), (mksh_uari_t)kshegid, 10);
	vp->flag |= INT_U;
	setint_n((vp = global("KSHGID")), (mksh_uari_t)kshgid, 10);
	vp->flag |= INT_U;
	/* this is needed globally */
	setint_n((vp_pipest = global(Tpipest)), 0, 10);
	/* unsigned integer, but avoid rndset call: done farther below */
	vp = global("RANDOM");
	vp->flag &= ~SPECIAL;
	setint_n(vp, 0, 10);
	vp->flag |= SPECIAL | INT_U;

	/* Set this before parsing arguments */
	Flag(FPRIVILEGED) = (kshuid != ksheuid || kshgid != kshegid) ? 2 : 0;

	/* record if monitor is set on command line (see j_init() in jobs.c) */
#ifndef MKSH_UNEMPLOYED
	Flag(FMONITOR) = 127;
#endif
	/* this to note if utf-8 mode is set on command line (see below) */
/*XXX this and the below can probably go away */
/*XXX UTFMODE should be set from env here if locale tracking, just keep it */
	UTFMODE = 2;

	if (!as_builtin) {
		argi = parse_args(argv, OF_CMDLINE, NULL);
		if (argi < 0)
			unwind(LERROR);
	}

/*XXX drop this and the entire cases below */
	/* process this later only, default to off (hysterical raisins) */
	utf_flag = UTFMODE;
	UTFMODE = 0;

	if (as_builtin) {
		/* auto-detect from environment variables, always */
		utf_flag = 3;
	} else if (Flag(FCOMMAND)) {
		s = pushs(SSTRINGCMDLINE, ATEMP);
		if (!(s->start = s->str = argv[argi++]))
			kerrf(KWF_ERR(1) | KWF_PREFIX | KWF_FILELINE |
			    KWF_TWOMSG | KWF_NOERRNO, Tdc, Treq_arg);
		while (*s->str) {
			if (ctype(*s->str, C_QUOTE))
				break;
			s->str++;
		}
		if (!*s->str)
			s->flags |= SF_MAYEXEC;
		s->str = s->start;
#ifdef MKSH_MIDNIGHTBSD01ASH_COMPAT
		/* compatibility to MidnightBSD 0.1 /bin/sh (kludge) */
		if (Flag(FSH) && argv[argi] && !strcmp(argv[argi], "--"))
			++argi;
#endif
		if (argv[argi])
			kshname = argv[argi++];
	} else if (argi < argc && !Flag(FSTDIN)) {
		s = pushs(SFILE, ATEMP);
#ifdef __OS2__
		/*
		 * A bug in OS/2 extproc (like shebang) handling makes
		 * it not pass the full pathname of a script, so we need
		 * to search for it. This changes the behaviour of a
		 * simple "mksh foo", but can't be helped.
		 */
		s->file = argv[argi++];
		if (search_access(s->file, X_OK) != 0)
			s->file = search_path(s->file, path, X_OK, NULL);
		if (!s->file || !*s->file)
			s->file = argv[argi - 1];
#else
		s->file = argv[argi++];
#endif
		s->u.shf = shf_open(s->file, O_RDONLY | O_MAYEXEC, 0,
		    SHF_MAPHI | SHF_CLEXEC);
		if (s->u.shf == NULL) {
			shl_stdout_ok = Nee;
			kwarnf(KWF_ERR(127) |
			    KWF_PREFIX | KWF_FILELINE | KWF_ONEMSG, s->file);
			unwind(LERROR);
		}
		kshname = s->file;
	} else {
		Flag(FSTDIN) = 1;
		s = pushs(SSTDIN, ATEMP);
		s->file = "<stdin>";
		s->u.shf = shf_fdopen(0, SHF_RD | can_seek(0),
		    NULL);
		if (isatty(0) && isatty(2)) {
			Flag(FTALKING) = Flag(FTALKING_I) = 1;
			/* The following only if isatty(0) */
			s->flags |= SF_TTY;
			s->u.shf->flags |= SHF_INTERRUPT;
			s->file = NULL;
		}
	}

	/* this bizarreness is mandated by POSIX */
	if (Flag(FTALKING) && fstat(0, &s_stdin) >= 0 &&
	    (S_ISCHR(s_stdin.st_mode) || S_ISFIFO(s_stdin.st_mode)))
		reset_nonblock(0);

	/* initialise job control */
	j_init();
	/* do this after j_init() which calls tty_init_state() */
	if (Flag(FTALKING)) {
		if (utf_flag == 2) {
#ifndef MKSH_ASSUME_UTF8
			/* auto-detect from locale or environment */
			utf_flag = 4;
#else /* this may not be an #elif */
#if MKSH_ASSUME_UTF8
			utf_flag = 1;
#else
			/* always disable UTF-8 (for interactive) */
			utf_flag = 0;
#endif
#endif
		}
#ifndef MKSH_NO_CMDLINE_EDITING
		x_init();
#endif
	}

#ifdef SIGWINCH
	sigtraps[SIGWINCH].flags |= TF_SHELL_USES;
	setsig(&sigtraps[SIGWINCH], x_sigwinch,
	    SS_RESTORE_ORIG|SS_FORCE|SS_SHTRAP);
#endif

	/*
	 * allocate a new array because otherwise, when we modify
	 * it in-place, ps(1) output changes; the meaning of argc
	 * here is slightly different as it excludes kshname, and
	 * we add a trailing NULL sentinel as well
	 */
	e->loc->argc = argc - argi;
	e->loc->argv = alloc2(e->loc->argc + 2, sizeof(char *), APERM);
	memcpy(&e->loc->argv[1], &argv[argi], e->loc->argc * sizeof(char *));
	e->loc->argv[e->loc->argc + 1] = NULL;
	if (as_builtin)
		e->loc->argv[0] = ccp;
	else {
		e->loc->argv[0] = kshname;
		getopts_reset(1);
	}

/*XXX to go away entirely with locale tracking */
	/* divine the initial state of the utf8-mode Flag */
	ccp = null;
	switch (utf_flag) {

#ifdef MKSH_EARLY_LOCALE_TRACKING
	/* not set on command line, not FTALKING */
	case 2:
#endif
	/* auto-detect from locale or environment */
	case 4:
#ifndef MKSH_EARLY_LOCALE_TRACKING
#if HAVE_POSIX_UTF8_LOCALE
		ccp = setlocale(LC_CTYPE, "");
		if (!isuc(ccp))
			ccp = nl_langinfo(CODESET);
		if (!isuc(ccp))
			ccp = null;
#endif
#endif
		/* FALLTHROUGH */

	/* auto-detect from environment */
	case 3:
#ifndef MKSH_EARLY_LOCALE_TRACKING
		/* these were imported from environ earlier */
		if (ccp == null)
			ccp = str_val(global("LC_ALL"));
		if (ccp == null)
			ccp = str_val(global("LC_CTYPE"));
		if (ccp == null)
			ccp = str_val(global("LANG"));
		UTFMODE = isuc(ccp);
#else
		recheck_ctype(); /*XXX probably unnecessary here */
#endif
		break;

#ifndef MKSH_EARLY_LOCALE_TRACKING
	/* not set on command line, not FTALKING */
	case 2:
#endif
	/* unknown values */
	default:
		utf_flag = 0;
		/* FALLTHROUGH */

	/* known values */
	case 1:
	case 0:
		UTFMODE = utf_flag;
		break;
	}

	/* Disable during .profile/ENV reading */
	restricted_shell |= Flag(FRESTRICTED);
	Flag(FRESTRICTED) = 0;
	errexit = Flag(FERREXIT);
	Flag(FERREXIT) = 0;

	/* save flags for "set +o" handling */
	memcpy(baseline_flags, shell_flags, sizeof(shell_flags));
	/* disable these because they have special handling */
	baseline_flags[(int)FPOSIX] = 0;
	baseline_flags[(int)FSH] = 0;
	/* ensure these always show up setting, for FPOSIX/FSH */
	baseline_flags[(int)FBRACEEXPAND] = 0;
	baseline_flags[(int)FUNNYCODE] = 0;
#if !defined(MKSH_SMALL) || defined(DEBUG)
	/* mark as initialised */
	baseline_flags[(int)FNFLAGS] = 1;
#endif
	rndpush(shell_flags, sizeof(shell_flags));
	rndset(hash(/*=current_wd*/ cp) ^ hash(argv[0]));
	if (as_builtin)
		goto skip_startup_files;

	/*
	 * Do this before profile/$ENV so that if it causes problems in them,
	 * user will know why things broke.
	 */
	if (!current_wd[0] && Flag(FTALKING))
		kwarnf(KWF_PREFIX | KWF_ONEMSG | KWF_NOERRNO,
		    "can't determine current directory");

	if (Flag(FLOGIN))
		include(MKSH_SYSTEM_PROFILE, NULL, Ja);
	if (Flag(FPRIVILEGED)) {
		include(MKSH_SUID_PROFILE, NULL, Ja);
		/* note whether -p was enabled during startup */
		if (Flag(FPRIVILEGED) == 1)
			/* allow set -p to setuid() later */
			Flag(FPRIVILEGED) = 3;
		else
			/* turn off -p if not set explicitly */
			change_flag(FPRIVILEGED, OF_INTERNAL, Nee);
		/* track shell-imposed changes */
		baseline_flags[(int)FPRIVILEGED] = Flag(FPRIVILEGED);
	} else {
		if (Flag(FLOGIN))
			include(substitute("$HOME/.profile", 0), NULL, Ja);
		if (Flag(FTALKING)) {
			cp = substitute("${ENV:-" MKSHRC_PATH "}", DOTILDE);
			if (cp[0] != '\0')
				include(cp, NULL, Ja);
		}
	}
	if (restricted_shell) {
		c_builtin(restr_com);
		/* After typeset command... */
		Flag(FRESTRICTED) = 1;
		/* track shell-imposed changes */
		baseline_flags[(int)FRESTRICTED] = 1;
	}
	Flag(FERREXIT) = errexit;

	if (Flag(FTALKING) && s)
		hist_init(s);
	else {
		/* set after ENV */
 skip_startup_files:
		Flag(FTRACKALL) = 1;
		/* track shell-imposed change (might lower surprise) */
		baseline_flags[(int)FTRACKALL] = 1;
	}

	alarm_init();

	*sp = s;
}

/* this indirection barrier reduces stack usage during normal operation */

int
main(int argc, const char *argv[])
{
	int rv;
	Source *s;

	main_init(argc, argv, &s);
	if (as_builtin) {
		rv = c_builtin(e->loc->argv) & 0xFF;
		exstat = rv;
		unwind(LEXIT);
		/* NOTREACHED */
	} else {
		rv = shell(s, 0) & 0xFF;
		/* NOTREACHED */
	}
	return (rv);
}

int
include(const char *name, const char **argv, Wahr intr_ok)
{
	Source *volatile s = NULL;
	struct shf *shf;
	const char **volatile old_argv;
	volatile int old_argc;
	int i;

	shf = shf_open(name, O_RDONLY | O_MAYEXEC, 0, SHF_MAPHI | SHF_CLEXEC);
	if (shf == NULL)
		return (-1);

	if (argv) {
		old_argv = e->loc->argv;
		old_argc = e->loc->argc;
	} else {
		old_argv = NULL;
		old_argc = 0;
	}
	newenv(E_INCL);
	if ((i = kshsetjmp(e->jbuf))) {
		quitenv(s ? s->u.shf : NULL);
		if (old_argv) {
			e->loc->argv = old_argv;
			e->loc->argc = old_argc;
		}
		switch (i) {
		case LRETURN:
		case LERROR:
		case LERREXT:
			/* see below */
			return (exstat & 0xFF);
		case LINTR:
			/*
			 * intr_ok is set if we are including .profile or $ENV.
			 * If user ^Cs out, we don't want to kill the shell...
			 */
			if (intr_ok && ((exstat & 0xFF) - 128) != SIGTERM)
				return (1);
			/* FALLTHROUGH */
		case LEXIT:
		case LLEAVE:
		case LSHELL:
			unwind(i);
			/* NOTREACHED */
		default:
			kerrf0(KWF_INTERNAL | KWF_ERR(0xFF) | KWF_NOERRNO,
			    Tunexpected_type, Tunwind, Tsource, i);
			/* NOTREACHED */
		}
	}
	if (argv) {
		e->loc->argv = argv;
		e->loc->argc = 0;
		while (argv[e->loc->argc + 1])
			++e->loc->argc;
	}
	s = pushs(SFILE, ATEMP);
	s->u.shf = shf;
	strdupx(s->file, name, ATEMP);
	i = shell(s, 1);
	quitenv(s->u.shf);
	if (old_argv) {
		e->loc->argv = old_argv;
		e->loc->argc = old_argc;
	}
	/* & 0xff to ensure value not -1 */
	return (i & 0xFF);
}

/* spawn a command into a shell optionally keeping track of the line number */
int
command(const char *comm, int line)
{
	Source *s, *sold = source;
	int rv;

	s = pushs(SSTRING, ATEMP);
	s->start = s->str = comm;
	s->line = line;
	rv = shell(s, 1);
	source = sold;
	return (rv);
}

/*
 * run the commands from the input source, returning status.
 */
int
shell(Source * volatile s, volatile int level)
{
	struct op *t;
	volatile Wahr wastty = isWahr(s->flags & SF_TTY);
	volatile kby attempts = 13;
	volatile Wahr interactive = (level == 0) && Flag(FTALKING);
	Source *volatile old_source = source;
	int i;

	newenv(level == 2 ? E_EVAL : E_PARSE);
	if (level == 2)
		e->flags |= EF_IN_EVAL;
	if (interactive)
		really_exit = Nee;
	switch ((i = kshsetjmp(e->jbuf))) {
	case 0:
		break;
	case LBREAK:
	case LCONTIN:
		/* assert: interactive == Nee */
		source = old_source;
		quitenv(NULL);
		if (level == 2) {
			/* keep on going */
			unwind(i);
			/* NOTREACHED */
		}
		kerrf0(KWF_INTERNAL | KWF_ERR(0xFF) | KWF_NOERRNO,
		    Tf_cant_s, Tshell, i == LBREAK ? Tbreak : Tcontinue);
		/* NOTREACHED */
	case LINTR:
		/* we get here if SIGINT not caught or ignored */
	case LERROR:
	case LERREXT:
	case LSHELL:
		if (interactive) {
			if (i == LINTR)
				shellf(Tnl);
			/*
			 * Reset any eof that was read as part of a
			 * multiline command.
			 */
			if (Flag(FIGNOREEOF) && s->type == SEOF && wastty)
				s->type = SSTDIN;
			/*
			 * Used by exit command to get back to
			 * top level shell. Kind of strange since
			 * interactive is set if we are reading from
			 * a tty, but to have stopped jobs, one only
			 * needs FMONITOR set (not FTALKING/SF_TTY)...
			 */
			/* toss any input we have so far */
			yyrecursive_pop(Ja);
			s->start = s->str = null;
			retrace_info = NULL;
			herep = heres;
			break;
		}
		/* FALLTHROUGH */
	case LEXIT:
	case LLEAVE:
	case LRETURN:
		source = old_source;
		quitenv(NULL);
		if (i == LERREXT && level == 2)
			return (exstat & 0xFF);
		/* keep on going */
		unwind(i);
		/* NOTREACHED */
	default:
		source = old_source;
		quitenv(NULL);
		kerrf0(KWF_INTERNAL | KWF_ERR(0xFF) | KWF_NOERRNO,
		    Tunexpected_type, Tunwind, Tshell, i);
		/* NOTREACHED */
	}
	while (/* CONSTCOND */ 1) {
		if (trap)
			runtraps(0);

		if (s->next == NULL) {
			if (Flag(FVERBOSE))
				s->flags |= SF_ECHO;
			else
				s->flags &= ~SF_ECHO;
		}
		if (interactive) {
			j_notify();
			set_prompt(PS1, s);
		}
		t = compile(s, Ja);
		if (interactive)
			histsave(&s->line, NULL, HIST_FLUSH, Ja);
		if (!t)
			goto source_no_tree;
		if (t->type == TEOF) {
			if (wastty && Flag(FIGNOREEOF) && --attempts > 0) {
				shellf("Use 'exit' to leave mksh\n");
				s->type = SSTDIN;
			} else if (wastty && !really_exit &&
			    j_stopped_running()) {
				really_exit = Ja;
				s->type = SSTDIN;
			} else {
				/*
				 * this for POSIX which says EXIT traps
				 * shall be taken in the environment
				 * immediately after the last command
				 * executed.
				 */
				if (level == 0)
					unwind(LEXIT);
				break;
			}
		} else if ((s->flags & SF_MAYEXEC) && t->type == TCOM)
			t->u.evalflags |= DOTCOMEXEC;
		if (!Flag(FNOEXEC) || (s->flags & SF_TTY))
			exstat = execute(t, 0, NULL) & 0xFF;

		if (t->type != TEOF && interactive && really_exit)
			really_exit = Nee;

 source_no_tree:
		reclaim();
	}
	source = old_source;
	quitenv(NULL);
	return (exstat & 0xFF);
}

/* return to closest error handler or shell(), exit if none found */
/* note: i MUST NOT be 0 */
void
unwind(int i)
{
	/* during eval, skip FERREXIT trap */
	if (i == LERREXT && (e->flags & EF_IN_EVAL))
		goto defer_traps;

	/* ordering for EXIT vs ERR is a bit odd (this is what AT&T ksh does) */
	if (i == LEXIT || ((i == LERROR || i == LERREXT || i == LINTR) &&
	    sigtraps[ksh_SIGEXIT].trap &&
	    (!Flag(FTALKING) || Flag(FERREXIT)))) {
		++trap_nested;
		runtrap(&sigtraps[ksh_SIGEXIT], trap_nested == 1);
		--trap_nested;
		i = LLEAVE;
	} else if (Flag(FERREXIT) && (i == LERROR || i == LERREXT || i == LINTR)) {
		++trap_nested;
		runtrap(&sigtraps[ksh_SIGERR], trap_nested == 1);
		--trap_nested;
		i = LLEAVE;
	}
 defer_traps:

	while (/* CONSTCOND */ 1) {
		switch (e->type) {
		case E_PARSE:
		case E_FUNC:
		case E_INCL:
		case E_LOOP:
		case E_ERRH:
		case E_EVAL:
			kshlongjmp(e->jbuf, i);
			/* NOTREACHED */
		case E_NONE:
			if (i == LINTR)
				e->flags |= EF_FAKE_SIGDIE;
			/* FALLTHROUGH */
		default:
			quitenv(NULL);
		}
	}
}

void
newenv(int type)
{
	struct env *ep;
	char *cp;

	reclim_trace();
	/*
	 * struct env includes ALLOC_ITEM for alignment constraints
	 * so first get the actually used memory, then assign it
	 */
	cp = alloc(sizeof(struct env) - sizeof(ALLOC_ITEM), ATEMP);
	/* undo what alloc() did to the malloc result address */
	ep = (void *)(cp - sizeof(ALLOC_ITEM));
	/* initialise public members of struct env (not the ALLOC_ITEM) */
	ainit(&ep->area);
	ep->oenv = e;
	ep->loc = e->loc;
	ep->savedfd = NULL;
	ep->temps = NULL;
	ep->yyrecursive_statep = NULL;
	ep->type = type;
	ep->flags = e->flags & EF_IN_EVAL;
	e = ep;
}

void
quitenv(struct shf *shf)
{
	struct env *ep = e;
	char *cp;
	int fd, i;

	yyrecursive_pop(Ja);
	while (ep->oenv && ep->oenv->loc != ep->loc)
		popblock();
	if (ep->savedfd != NULL) {
		for (fd = 0; fd < NUFILE; fd++)
			if ((i = SAVEDFD(ep, fd)))
				restfd(fd, i);
		if (SAVEDFD(ep, 2))
			/* Clear any write errors */
			shf_reopen(2, SHF_WR, shl_out);
	}
	if (ep->type == E_EXEC) {
		/* could be set, would be reclaim()ed below */
		builtin_argv0 = NULL;
	}
	/*
	 * Bottom of the stack.
	 * Either main shell is exiting or cleanup_parents_env() was called.
	 */
	if (ep->oenv == NULL) {
		if (ep->type == E_NONE) {
			/* Main shell exiting? */
#if HAVE_PERSISTENT_HISTORY
			if (Flag(FTALKING))
				hist_finish();
#endif
			j_exit();
			if (ep->flags & EF_FAKE_SIGDIE) {
				int sig = (exstat & 0xFF) - 128;

				/*
				 * ham up our death a bit (AT&T ksh
				 * only seems to do this for SIGTERM)
				 * Don't do it for SIGQUIT, since we'd
				 * dump a core..
				 */
				if ((sig == SIGINT || sig == SIGTERM) &&
				    (kshpgrp == kshpid)) {
					setsig(&sigtraps[sig], SIG_DFL,
					    SS_RESTORE_CURR | SS_FORCE);
					kill(0, sig);
				}
			}
		}
		if (shf)
			shf_close(shf);
		reclaim();
#ifdef DEBUG_LEAKS
#ifndef MKSH_NO_CMDLINE_EDITING
		x_done();
#endif
#ifndef MKSH_NOPROSPECTOFWORK
		/* block at least SIGCHLD during/after afreeall */
		sigprocmask(SIG_BLOCK, &sm_sigchld, NULL);
#endif
		afreeall(APERM);
#if HAVE_GET_CURRENT_DIR_NAME
		free_gnu_gcdn(getwd_bufp);
#endif
		for (fd = 3; fd < NUFILE; fd++)
			if ((i = fcntl(fd, F_GETFD, 0)) != -1 &&
			    (i & FD_CLOEXEC))
				close(fd);
		close(2);
		close(1);
		close(0);
#endif
		exit(exstat & 0xFF);
	}
	if (shf)
		shf_close(shf);
	reclaim();

	e = e->oenv;

	/* free the struct env - tricky due to the ALLOC_ITEM inside */
	cp = (void *)ep;
	afree(cp + sizeof(ALLOC_ITEM), ATEMP);
}

/* Called after a fork to cleanup stuff left over from parents environment */
void
cleanup_parents_env(void)
{
	struct env *ep;
	int fd;

	/*
	 * Don't clean up temporary files - parent will probably need them.
	 * Also, can't easily reclaim memory since variables, etc. could be
	 * anywhere.
	 */

	/* close all file descriptors hiding in savedfd */
	for (ep = e; ep; ep = ep->oenv) {
		if (ep->savedfd) {
			for (fd = 0; fd < NUFILE; fd++)
				if (FDSVNUM(ep, fd) > (kui)FDBASE)
					close((int)FDSVNUM(ep, fd));
			afree(ep->savedfd, &ep->area);
			ep->savedfd = NULL;
		}
#ifdef DEBUG_LEAKS
		if (ep->type != E_NONE)
			ep->type = E_GONE;
#endif
	}
#ifndef DEBUG_LEAKS
	e->oenv = NULL;
#endif
}

/* Called just before an execve cleanup stuff temporary files */
void
cleanup_proc_env(void)
{
	struct env *ep;

	for (ep = e; ep; ep = ep->oenv)
		remove_temps(ep->temps);
}

/* remove temp files and free ATEMP Area */
static void
reclaim(void)
{
	struct block *l;

	while ((l = e->loc) && (!e->oenv || e->oenv->loc != l)) {
		e->loc = l->next;
		afreeall(&l->area);
	}

	remove_temps(e->temps);
	e->temps = NULL;

	/*
	 * if the memory backing source is reclaimed, things
	 * will end up badly when a function expecting it to
	 * be valid is run; a NULL pointer is easily debugged
	 */
	if (source && source->areap == &e->area)
		source = NULL;
	retrace_info = NULL;
	afreeall(&e->area);
}

static void
remove_temps(struct temp *tp)
{
	while (tp) {
		if (tp->pid == procpid)
			unlink(tp->tffn);
		tp = tp->next;
	}
}

/*
 * Initialise tty_fd. Used for tracking the size of the terminal,
 * saving/resetting tty modes upon foreground job completion, and
 * for setting up the tty process group. Return values:
 *	0 = got controlling tty
 *	1 = got terminal but no controlling tty
 *	2 = cannot find a terminal
 *	3 = cannot dup fd
 *	4 = cannot make fd close-on-exec
 * An existing tty_fd is cached if no "better" one could be found,
 * i.e. if tty_devtty was already set or the new would not set it.
 */
int
tty_init_fd(void)
{
	int fd, rv, eno = 0;
	Wahr do_close = Nee, is_devtty = Ja;

	if (tty_devtty) {
		/* already got a tty which is /dev/tty */
		return (0);
	}

#ifdef _UWIN
	/*XXX imake style */
	if (isatty(3)) {
		/* fd 3 on UWIN _is_ /dev/tty (or our controlling tty) */
		fd = 3;
		goto got_fd;
	}
#endif
	if ((fd = open("/dev/tty", O_RDWR, 0)) >= 0) {
		do_close = Ja;
		goto got_fd;
	}
	eno = errno;

	if (tty_fd >= 0) {
		/* already got a non-devtty one */
		rv = 1;
		goto out;
	}
	is_devtty = Nee;

	if (isatty((fd = 0)) || isatty((fd = 2)))
		goto got_fd;
	/* cannot find one */
	rv = 2;
	/* assert: do_close == Nee */
	goto out;

 got_fd:
	if ((rv = fcntl(fd, F_DUPFD, FDBASE)) < 0) {
		eno = errno;
		rv = 3;
		goto out;
	}
	if (fcntl(rv, F_SETFD, FD_CLOEXEC) < 0) {
		eno = errno;
		close(rv);
		rv = 4;
		goto out;
	}
	tty_fd = rv;
	tty_devtty = is_devtty;
	rv = eno = 0;
 out:
	if (do_close)
		close(fd);
	errno = eno;
	return (rv);
}

/* printf to shl_out (stderr) with flush */
void
shellf(const char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	shf_vfprintf(shl_out, fmt, va);
	va_end(va);
	shf_flush(shl_out);
}

/* printf to shl_stdout (stdout) */
void
shprintf(const char *fmt, ...)
{
	va_list va;

	if (!shl_stdout_ok)
		kerrf(KWF_INTERNAL | KWF_ERR(0xFF) | KWF_ONEMSG | KWF_NOERRNO,
		    "shl_stdout not valid");
	va_start(va, fmt);
	shf_vfprintf(shl_stdout, fmt, va);
	va_end(va);
}

/* test if we can seek backwards fd (returns 0 or SHF_UNBUF) */
int
can_seek(int fd)
{
	struct stat statb;

	return (fstat(fd, &statb) == 0 && !S_ISREG(statb.st_mode) ?
	    SHF_UNBUF : 0);
}

#ifdef DF
int shl_dbg_fd;
#define NSHF_IOB 4
#else
#define NSHF_IOB 3
#endif
struct shf shf_iob[NSHF_IOB];

/* pre-initio() */
void
initio(void)
{
#ifdef DF
	const char *lfp;
#endif

	/* force buffer allocation */
	shf_fdopen(1, SHF_WR, shl_stdout);
	shf_fdopen(2, SHF_WR, shl_out);
	shf_fdopen(2, SHF_WR, shl_xtrace);
	initio_done = Ja;
#ifdef DF
	if ((lfp = getenv("SDMKSH_PATH")) == NULL) {
		if ((lfp = getenv("HOME")) == NULL || !mksh_abspath(lfp))
			kerrf(KWF_INTERNAL | KWF_ERR(0xFF) | KWF_ONEMSG |
			    KWF_NOERRNO, "can't get home directory");
		strpathx(lfp, lfp, "mksh-dbg.txt", 1);
	}

	if ((shl_dbg_fd = open(lfp, O_WRONLY | O_APPEND | O_CREAT, 0600)) < 0)
		kerrf(KWF_INTERNAL | KWF_ERR(0xFF) | KWF_TWOMSG,
		    lfp, "can't open debug output file");
	if (shl_dbg_fd < FDBASE) {
		int nfd;

		if ((nfd = fcntl(shl_dbg_fd, F_DUPFD, FDBASE)) == -1)
			kerrf(KWF_INTERNAL | KWF_ERR(0xFF) | KWF_ONEMSG,
			    "can't dup debug output file");
		close(shl_dbg_fd);
		shl_dbg_fd = nfd;
	}
	fcntl(shl_dbg_fd, F_SETFD, FD_CLOEXEC);
	shf_fdopen(shl_dbg_fd, SHF_WR, shl_dbg);
	DF("=== open ===");
	initio_done = 2;
#endif
}

/* A dup2() with error checking */
int
ksh_dup2(int ofd, int nfd, Wahr errok)
{
	int rv;

	if (((rv = dup2(ofd, nfd)) < 0) && !errok && (errno != EBADF))
		kerrf0(KWF_ERR(1) | KWF_PREFIX | KWF_FILELINE,
		    Ttoo_many_files, ofd, nfd);

#ifdef __ultrix
	/*XXX imake style */
	if (rv >= 0)
		fcntl(nfd, F_SETFD, 0);
#endif

	return (rv);
}

/*
 * Move fd from user space (0 <= fd < FDBASE) to shell space (fd >= FDBASE)
 * set moved fd’s close-on-exec flag (see sh.h for FDBASE).
 */
int
savefd(int fd)
{
	int nfd = fd;

	errno = 0;
	if (fd < FDBASE && (nfd = fcntl(fd, F_DUPFD, FDBASE)) < 0 &&
	    (errno == EBADF || errno == EPERM))
		return (-1);
	if (nfd < FDBASE || nfd > (int)(kui)FDMAXNUM)
		kerrf0(KWF_ERR(1) | KWF_PREFIX | KWF_FILELINE,
		    Ttoo_many_files, fd, nfd);
	if (fcntl(nfd, F_SETFD, FD_CLOEXEC) == -1)
		kwarnf0(KWF_INTERNAL | KWF_WARNING, Tcloexec_failed,
		    "set", nfd);
	return (nfd);
}

void
restfd(int fd, int ofd)
{
	if (fd == 2)
		shf_flush(&shf_iob[/* fd */ 2]);
	if (ofd < 0)
		/* original fd closed */
		close(fd);
	else if (fd != ofd) {
		/*XXX: what to do if this dup fails? */
		ksh_dup2(ofd, fd, Ja);
		close(ofd);
	}
}

void
openpipe(int *pv)
{
	int lpv[2];

	if (pipe(lpv) < 0)
		kerrf(KWF_ERR(1) | KWF_PREFIX | KWF_FILELINE | KWF_ONEMSG,
		    "pipe");
	pv[0] = savefd(lpv[0]);
	if (pv[0] != lpv[0])
		close(lpv[0]);
	pv[1] = savefd(lpv[1]);
	if (pv[1] != lpv[1])
		close(lpv[1]);
#ifdef __OS2__
	setmode(pv[0], O_BINARY);
	setmode(pv[1], O_BINARY);
#endif
}

void
closepipe(int *pv)
{
	close(pv[0]);
	close(pv[1]);
}

/*
 * Called by iosetup() (deals with 2>&4, etc.), c_read, c_print to turn
 * a string (the X in 2>&X, read -uX, print -uX) into a file descriptor.
 */
int
check_fd(const char *name, int mode, const char **emsgp)
{
	int fd, fl;

	if (!name[0] || name[1])
		goto illegal_fd_name;
	if (name[0] == 'p')
		return (coproc_getfd(mode, emsgp));
	if (!ctype(name[0], C_DIGIT)) {
 illegal_fd_name:
		if (emsgp)
			*emsgp = "illegal file descriptor name";
		errno = EINVAL;
		return (-1);
	}

	if ((fl = fcntl((fd = ksh_numdig(name[0])), F_GETFL, 0)) < 0) {
		if (emsgp)
			*emsgp = "bad file descriptor";
		return (-1);
	}
	fl &= O_ACCMODE;
	/*
	 * X_OK is a kludge to disable this check for dups (x<&1):
	 * historical shells never did this check (XXX don't know what
	 * POSIX has to say).
	 */
	if (!(mode & X_OK) && fl != O_RDWR && (
	    ((mode & R_OK) && fl != O_RDONLY) ||
	    ((mode & W_OK) && fl != O_WRONLY))) {
		if (emsgp)
			*emsgp = (fl == O_WRONLY) ?
			    "fd not open for reading" :
			    "fd not open for writing";
#ifdef ENXIO
		errno = ENXIO;
#else
		errno = EBADF;
#endif
		return (-1);
	}
	return (fd);
}

/* Called once from main */
void
coproc_init(void)
{
	coproc.read = coproc.readw = coproc.write = -1;
	coproc.njobs = 0;
	coproc.id = 0;
}

/* Called by c_read() when eof is read - close fd if it is the co-process fd */
void
coproc_read_close(int fd)
{
	if (coproc.read >= 0 && fd == coproc.read) {
		coproc_readw_close(fd);
		close(coproc.read);
		coproc.read = -1;
	}
}

/*
 * Called by c_read() and by iosetup() to close the other side of the
 * read pipe, so reads will actually terminate.
 */
void
coproc_readw_close(int fd)
{
	if (coproc.readw >= 0 && coproc.read >= 0 && fd == coproc.read) {
		close(coproc.readw);
		coproc.readw = -1;
	}
}

/*
 * Called by c_print when a write to a fd fails with EPIPE and by iosetup
 * when co-process input is dup'd
 */
void
coproc_write_close(int fd)
{
	if (coproc.write >= 0 && fd == coproc.write) {
		close(coproc.write);
		coproc.write = -1;
	}
}

/*
 * Called to check for existence of/value of the co-process file descriptor.
 * (Used by check_fd() and by c_read/c_print to deal with -p option).
 */
int
coproc_getfd(int mode, const char **emsgp)
{
	int fd = (mode & R_OK) ? coproc.read : coproc.write;

	if (fd >= 0)
		return (fd);
	if (emsgp)
		*emsgp = "no coprocess";
	errno = EBADF;
	return (-1);
}

/*
 * called to close file descriptors related to the coprocess (if any)
 * Should be called with SIGCHLD blocked.
 */
void
coproc_cleanup(int reuse)
{
	/* This to allow co-processes to share output pipe */
	if (!reuse || coproc.readw < 0 || coproc.read < 0) {
		if (coproc.read >= 0) {
			close(coproc.read);
			coproc.read = -1;
		}
		if (coproc.readw >= 0) {
			close(coproc.readw);
			coproc.readw = -1;
		}
	}
	if (coproc.write >= 0) {
		close(coproc.write);
		coproc.write = -1;
	}
}

struct temp *
maketemp(Area *ap, Temp_type type, struct temp **tlist)
{
	char *cp;
	size_t len;
	int i, j;
	struct temp *tp;
	const char *dir;
	struct stat sb;

	dir = tmpdir ? tmpdir : MKSH_DEFAULT_TMPDIR;
	/* add "/shXXXXXX.tmp" plus NUL */
	len = strlen(dir);
	cp = alloc1(offsetof(struct temp, tffn[0]) + 14U, len, ap);

	tp = (void *)cp;
	tp->shf = NULL;
	tp->pid = procpid;
	tp->type = type;

	cp += offsetof(struct temp, tffn[0]);
	memcpy(cp, dir, len);
	cp += len;
	memstr(cp, "/shXXXXXX.tmp");

	if (stat(dir, &sb) || !S_ISDIR(sb.st_mode))
		goto maketemp_out;

	/* point to the first of six Xes */
	cp += 3;

	/* cyclically attempt to open a temporary file */
	do {
		/* generate random part of filename */
		len = 0;
		do {
			cp[len++] = digits_lc[rndget() % 36];
		} while (len < 6);

		/* check if this one works */
		if ((i = binopen3(tp->tffn, O_CREAT | O_EXCL | O_RDWR,
		    0600)) < 0 && errno != EEXIST)
			goto maketemp_out;
	} while (i < 0);

	if (type == TT_FUNSUB) {
		/* map us high and mark as close-on-exec */
		if ((j = savefd(i)) != i) {
			close(i);
			i = j;
		}

		/* operation mode for the shf */
		j = SHF_RD;
	} else
		j = SHF_WR;

	/* shf_fdopen cannot fail, so no fd leak */
	tp->shf = shf_fdopen(i, j, NULL);

 maketemp_out:
	tp->next = *tlist;
	*tlist = tp;
	return (tp);
}

/*
 * We use a similar collision resolution algorithm as Python 2.5.4
 * but with a slightly tweaked implementation written from scratch.
 */

#define	INIT_TBLSHIFT	3	/* initial table shift (2^3 = 8) */
#define PERTURB_SHIFT	5	/* see Python 2.5.4 Objects/dictobject.c */

static void tgrow(struct table *);
static int tnamecmp(const void *, const void *);

/* pre-initio() tp->tbls=NULL tp->tshift=INIT_TBLSHIFT-1 */
static void
tgrow(struct table *tp)
{
	size_t i, j, osize, mask, perturb;
	struct tbl *tblp, **pp;
	struct tbl **ntblp, **otblp = tp->tbls;

	if (tp->tshift > 29)
		kerrf(KWF_INTERNAL | KWF_ERR(0xFF) | KWF_ONEMSG | KWF_NOERRNO,
		    "hash table size limit reached");

	/* calculate old size, new shift and new size */
	osize = (size_t)1 << (tp->tshift++);
	i = osize << 1;

	ntblp = alloc2(i, sizeof(struct tbl *), tp->areap);
	/* multiplication cannot overflow: alloc2 checked that */
	memset(ntblp, 0, i * sizeof(struct tbl *));

	/* table can get very full when reaching its size limit */
	tp->nfree = (tp->tshift == 30) ? 0x3FFF0000UL :
	    /* but otherwise, only 75% */
	    ((i * 3) / 4);
	tp->tbls = ntblp;
	if (otblp == NULL)
		return;

	mask = i - 1;
	for (i = 0; i < osize; i++)
		if ((tblp = otblp[i]) != NULL) {
			if ((tblp->flag & DEFINED)) {
				/* search for free hash table slot */
				j = perturb = tblp->ua.hval;
				goto find_first_empty_slot;
 find_next_empty_slot:
				j = (j << 2) + j + perturb + 1;
				perturb >>= PERTURB_SHIFT;
 find_first_empty_slot:
				pp = &ntblp[j & mask];
				if (*pp != NULL)
					goto find_next_empty_slot;
				/* found an empty hash table slot */
				*pp = tblp;
				tp->nfree--;
			} else if (!(tblp->flag & FINUSE)) {
				afree(tblp, tp->areap);
			}
		}
	afree(otblp, tp->areap);
}

/* pre-initio() initshift=0 */
void
ktinit(Area *ap, struct table *tp, kby initshift)
{
	tp->areap = ap;
	tp->tbls = NULL;
	tp->tshift = ((initshift > INIT_TBLSHIFT) ?
	    initshift : INIT_TBLSHIFT) - 1;
	tgrow(tp);
}

/* table, name (key) to search for, hash(name), rv pointer to tbl ptr */
struct tbl *
ktscan(struct table *tp, const char *name, k32 h, struct tbl ***ppp)
{
	size_t j, perturb, mask;
	struct tbl **pp, *p;

	mask = ((size_t)1 << (tp->tshift)) - 1;
	/* search for hash table slot matching name */
	j = perturb = h;
	goto find_first_slot;
 find_next_slot:
	j = (j << 2) + j + perturb + 1;
	perturb >>= PERTURB_SHIFT;
 find_first_slot:
	pp = &tp->tbls[j & mask];
	if ((p = *pp) != NULL && (p->ua.hval != h || !(p->flag & DEFINED) ||
	    strcmp(p->name, name)))
		goto find_next_slot;
	/* p == NULL if not found, correct found entry otherwise */
	if (ppp)
		*ppp = pp;
	return (p);
}

/* table, name (key) to enter, hash(n) */
struct tbl *
ktenter(struct table *tp, const char *n, k32 h)
{
	struct tbl **pp, *p;
	size_t len;

 Search:
	if ((p = ktscan(tp, n, h, &pp)))
		return (p);

	if (tp->nfree == 0) {
		/* too full */
		tgrow(tp);
		goto Search;
	}

	/* create new tbl entry */
	len = strlen(n) + 1;
	p = alloc1(offsetof(struct tbl, name[0]), len, tp->areap);
	p->flag = 0;
	p->type = 0;
	p->areap = tp->areap;
	p->ua.hval = h;
	p->u2.field = 0;
	p->u.array = NULL;
	memcpy(p->name, n, len);

	/* enter in tp->tbls */
	tp->nfree--;
	*pp = p;
	return (p);
}

void
ktwalk(struct tstate *ts, struct table *tp)
{
	ts->left = (size_t)1 << (tp->tshift);
	ts->next = tp->tbls;
}

struct tbl *
ktnext(struct tstate *ts)
{
	while (--ts->left >= 0) {
		struct tbl *p = *ts->next++;
		if (p != NULL && (p->flag & DEFINED))
			return (p);
	}
	return (NULL);
}

static int
tnamecmp(const void *p1, const void *p2)
{
	const struct tbl *a = *((const struct tbl * const *)p1);
	const struct tbl *b = *((const struct tbl * const *)p2);

	return (ascstrcmp(a->name, b->name));
}

struct tbl **
ktsort(struct table *tp)
{
	size_t i;
	struct tbl **p, **sp, **dp;

	/*
	 * since the table is never entirely full, no need to reserve
	 * additional space for the trailing NULL appended below
	 */
	i = (size_t)1 << (tp->tshift);
	p = alloc2(i, sizeof(struct tbl *), ATEMP);
	sp = tp->tbls;		/* source */
	dp = p;			/* dest */
	while (i--)
		if ((*dp = *sp++) != NULL && (((*dp)->flag & DEFINED) ||
		    ((*dp)->flag & ARRAY)))
			dp++;
	qsort(p, (i = dp - p), sizeof(struct tbl *), tnamecmp);
	p[i] = NULL;
	return (p);
}

#ifdef SIGWINCH
static void
x_sigwinch(int sig MKSH_A_UNUSED)
{
	/* this runs inside interrupt context, with errno saved */

	got_winch = 1;
}
#endif

#ifdef DF
void
DF(const char *fmt, ...)
{
	va_list args;
	struct timeval tv;
	mirtime_mjd mjd;

	mksh_lockfd(shl_dbg_fd);
	mksh_TIME(tv);
	timet2mjd(&mjd, tv.tv_sec);
	shf_fprintf(shl_dbg, "[%02u:%02u:%02u (%u) %u.%06u] ",
	    (unsigned)mjd.sec / 3600U, ((unsigned)mjd.sec / 60U) % 60U,
	    (unsigned)mjd.sec % 60U, (unsigned)getpid(),
	    (unsigned)tv.tv_sec, (unsigned)tv.tv_usec);
	va_start(args, fmt);
	shf_vfprintf(shl_dbg, fmt, args);
	va_end(args);
	shf_putc('\n', shl_dbg);
	shf_flush(shl_dbg);
	mksh_unlkfd(shl_dbg_fd);
}
#endif

void
x_mkraw(int fd, mksh_ttyst *ocb, Wahr forread)
{
	mksh_ttyst cb;

	if (ocb)
		mksh_tcget(fd, ocb);
	else
		ocb = &tty_state;
#ifdef FLUSHO
	ocb->c_lflag &= ~(FLUSHO);
#endif

	cb = *ocb;
	cb.c_iflag &= ~(IGNPAR | PARMRK | INLCR | IGNCR | ICRNL | ISTRIP);
	cb.c_iflag |= BRKINT;
	if (forread)
		cb.c_lflag &= ~(ICANON);
	else
		cb.c_lflag &= ~(ISIG | ICANON | ECHO);
#if defined(VLNEXT)
	/* OSF/1 processes lnext when ~icanon */
	KSH_DOVDIS(cb.c_cc[VLNEXT]);
#endif
	/* SunOS 4.1.x and OSF/1 process discard(flush) when ~icanon */
#if defined(VDISCARD)
	KSH_DOVDIS(cb.c_cc[VDISCARD]);
#endif
	cb.c_cc[VTIME] = 0;
	cb.c_cc[VMIN] = 1;

	mksh_tcset(fd, &cb);
}

#ifdef MKSH_ENVDIR
#if HAVE_POSIX_UTF8_LOCALE
# error MKSH_ENVDIR has not been adapted to work with POSIX locale!
#else
static void
init_environ(void)
{
	char *xp;
	ssize_t n;
	XString xs;
	struct shf *shf;
	DIR *dirp;
	struct dirent *dent;

	if ((dirp = opendir(MKSH_ENVDIR)) == NULL) {
		kwarnf(KWF_PREFIX | KWF_TWOMSG, MKSH_ENVDIR,
		    "can't read environment");
		return;
	}
	XinitN(xs, 256, ATEMP);
 read_envfile:
	errno = 0;
	if ((dent = readdir(dirp)) != NULL) {
		if (skip_varname(dent->d_name, Ja)[0] == '\0') {
			strpathx(xp, MKSH_ENVDIR, dent->d_name, 1);
			if (!(shf = shf_open(xp, O_RDONLY, 0, 0))) {
				kwarnf(KWF_PREFIX | KWF_THREEMSG, MKSH_ENVDIR,
				    dent->d_name, "can't read environment");
				goto read_envfile;
			}
			afree(xp, ATEMP);
			n = strlen(dent->d_name);
			xp = Xstring(xs, xp);
			XcheckN(xs, xp, n + 32);
			memcpy(xp, dent->d_name, n);
			xp += n;
			*xp++ = '=';
			while ((n = shf_read(xp, Xnleft(xs, xp), shf)) > 0) {
				xp += n;
				if (Xnleft(xs, xp) <= 0)
					XcheckN(xs, xp, 128);
			}
			if (n < 0) {
				kwarnf(KWF_VERRNO | KWF_PREFIX | KWF_THREEMSG,
				    shf_errno(shf), MKSH_ENVDIR,
				    dent->d_name, "can't read environment");
			} else {
				*xp = '\0';
				rndpush(Xstring(xs, xp), Xlength(xs, xp));
				xp = Xstring(xs, xp);
				typeset(xp, IMPORT | EXPORT, 0, 0, 0);
			}
			shf_close(shf);
		}
		goto read_envfile;
	} else if (errno)
		kwarnf(KWF_PREFIX | KWF_TWOMSG, MKSH_ENVDIR,
		    "can't read environment");
	closedir(dirp);
	Xfree(xs, xp);
}
#endif
#else
extern char **environ;

static void
init_environ(void)
{
	const char **wp;

	if (environ == NULL)
		return;

	wp = (const char **)environ;
	while (*wp != NULL) {
		rndpush(*wp, strlen(*wp));
		typeset(*wp, IMPORT | EXPORT, 0, 0, 0);
#ifdef MKSH_EARLY_LOCALE_TRACKING
		if (isch((*wp)[0], 'L') && (
		    (isch((*wp)[1], 'C') && isch((*wp)[2], '_')) ||
		    !strcmp(*wp, "LANG"))) {
			const char **P;

			/* remove LC_* / LANG from own environment */
			P = wp;
			while ((*P = *(P + 1)))
				++P;
			/* now setlocale with "" will use the default locale */
			/* matching the user expectation wrt passed-in vars */
		} else
			++wp;
#else
		++wp;
#endif
	}
}
#endif

#ifdef MKSH_EARLY_LOCALE_TRACKING
void
recheck_ctype(void)
{
	const char *ccp;
#if !HAVE_POSIX_UTF8_LOCALE
	const char *cdp;
#endif

	/* determine active LC_CTYPE value */
	ccp = str_val(global("LC_ALL"));
	if (!*ccp)
		ccp = str_val(global("LC_CTYPE"));
	if (!*ccp)
		ccp = str_val(global("LANG"));
#if !HAVE_POSIX_UTF8_LOCALE
	if (!*ccp)
		ccp = MKSH_DEFAULT_LOCALE;
#endif

	/* determine codeset used */
#if HAVE_POSIX_UTF8_LOCALE
	errno = EINVAL;
	if (!setlocale(LC_CTYPE, ccp)) {
		kwarnf(KWF_PREFIX | KWF_FILELINE | KWF_ONEMSG, "setlocale");
		return;
	}
	ccp = nl_langinfo(CODESET);
#else
	/* tacked on to a locale name or just a codeset? */
	if ((cdp = cstrchr(ccp, '.')))
		ccp = cdp + 1;
#endif

	/* see whether it’s UTF-8 */
	UTFMODE = 0;
	if (!isCh(ccp[0], 'U', 'u') ||
	    !isCh(ccp[1], 'T', 't') ||
	    !isCh(ccp[2], 'F', 'f'))
		return;
	ccp += isch(ccp[3], '-') ? 4 : 3;
	if (!isch(*ccp, '8'))
		return;
	++ccp;
	/* verify nothing untoward trails the string */
#if !HAVE_POSIX_UTF8_LOCALE
	if (cdp) {
		/* tacked onto a locale name */
		if (*ccp && !isch(*ccp, '@'))
			return;
	} else
	  /* OSX has a "UTF-8" locale… */
#endif
	/* just a codeset so require EOS */
	  if (*ccp != '\0')
		return;
	/* positively identified as UTF-8 */
	UTFMODE = 1;
}
#endif

#if !HAVE_MEMMOVE
void *
rpl_memmove(void *dst, const void *src, size_t len)
{
	const unsigned char *s = src;
	unsigned char *d = dst;

	if (len) {
		if (src < dst) {
			s += len;
			d += len;
			while (len--)
				*--d = *--s;
		} else
			while (len--)
				*d++ = *s++;
	}
	return (dst);
}
#endif

const char *
ksh_getwd(void)
{
#if HAVE_GET_CURRENT_DIR_NAME
	free_gnu_gcdn(getwd_bufp);
	getwd_bufp = get_current_dir_name();
	if (getwd_bufp && !mksh_abspath(getwd_bufp)) {
		free_gnu_gcdn(getwd_bufp);
		getwd_bufp = NULL;
		errno = EACCES;
	}
#else
 redo:
	if (getcwd(getwd_bufp, getwd_bufsz)) {
		if (mksh_abspath(getwd_bufp))
			goto done;
		errno = EACCES;
		return (NULL);
	}
	if (errno == ERANGE) {
		if (notoktomul(getwd_bufsz, 2U)) {
			errno = ENAMETOOLONG;
			return (NULL);
		}
		getwd_bufsz <<= 1;
		getwd_bufp = aresize(getwd_bufp, getwd_bufsz + 1U, APERM);
		getwd_bufp[getwd_bufsz] = '\0';
		goto redo;
	}
	return (NULL);
 done:
#endif
	return (getwd_bufp);
}

#ifdef DEBUG
#if 1 /* enforce, 0 to find out */
static void
reclim_trace(void)
{
	struct env *ep = e;
	unsigned int nenv = 0;

 count:
	if (++nenv == 128U) {
		kerrf(KWF_INTERNAL | KWF_ERR(0xFF) | KWF_PREFIX |
		    KWF_FILELINE | KWF_ONEMSG | KWF_NOERRNO,
		    "\"mksh/Build.sh -g\" recursion limit exceeded");
	} else if (ep) {
		ep = ep->oenv;
		goto count;
	}
}
#elif defined(SIG_ATOMIC_MAX) && (SIG_ATOMIC_MAX >= 0x7FFF)
#include <syslog.h>
static void reclim_atexit(void);

static volatile sig_atomic_t reclim_v = 0;
static kby reclim_warned = 0;

static void
reclim_trace(void)
{
	struct env *ep = e;
	unsigned int nenv = 0;

	if (!reclim_v)
		atexit(reclim_atexit);
 count:
	if (nenv == SIG_ATOMIC_MAX) {
		syslog(LOG_WARNING, "reclim_trace: SIG_ATOMIC_MAX reached");
		reclim_v = SIG_ATOMIC_MAX;
		return;
	}
	if (!++nenv) {
		--nenv;
		if (!reclim_warned) {
			syslog(LOG_WARNING, "reclim_trace: %s reached",
			    "UINT_MAX");
			reclim_warned = 1;
		}
	} else if (ep) {
		ep = ep->oenv;
		goto count;
	}

	if ((mbiHUGE_U)nenv >= (mbiHUGE_U)(SIG_ATOMIC_MAX)) {
		if (!reclim_warned) {
			syslog(LOG_WARNING, "reclim_trace: %s reached",
			    "SIG_ATOMIC_MAX");
			reclim_warned = 1;
		}
		reclim_v = SIG_ATOMIC_MAX;
		return;
	}
	if ((sig_atomic_t)nenv > reclim_v)
		reclim_v = (sig_atomic_t)nenv;
}

static void
reclim_atexit(void)
{
	unsigned int nenv = (unsigned int)reclim_v;

	if (nenv > 32U)
		syslog(LOG_DEBUG, "max recursion limit: %u", nenv);
}
#else /* missing suitable large sig_atomic_t */
static void
reclim_trace(void)
{
}
#endif /* missing suitable large sig_atomic_t */
#endif /* DEBUG */
