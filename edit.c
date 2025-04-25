/*	$OpenBSD: edit.c,v 1.41 2015/09/01 13:12:31 tedu Exp $	*/
/*	$OpenBSD: edit.h,v 1.9 2011/05/30 17:14:35 martynas Exp $	*/
/*	$OpenBSD: emacs.c,v 1.52 2015/09/10 22:48:58 nicm Exp $	*/
/*	$OpenBSD: vi.c,v 1.30 2015/09/10 22:48:58 nicm Exp $	*/

/*-
 * Copyright (c) 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010,
 *		 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018,
 *		 2019, 2020, 2021, 2022
 *	mirabilos <m$(date +%Y)@mirbsd.de>
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

#ifndef MKSH_NO_CMDLINE_EDITING

__RCSID("$MirOS: src/bin/mksh/edit.c,v 1.401 2025/04/25 23:14:53 tg Exp $");

/*
 * in later versions we might use libtermcap for this, but since external
 * dependencies are problematic, this has not yet been decided on; another
 * good string is KSH_ESC_STRING "c" except on hardware terminals like the
 * DEC VT420 which do a full power cycle then...
 */
#ifndef MKSH_CLS_STRING
#define MKSH_CLS_STRING		KSH_ESC_STRING "[;H" KSH_ESC_STRING "[J"
#endif

#if !defined(MKSH_SMALL) || !MKSH_S_NOVI
static const char ctrl_x_e[] = "fc -e \"${VISUAL:-${EDITOR:-vi}}\" --";
#endif

/* tty driver characters we are interested in */
#define EDCHAR_DISABLED	0xFFFFU
#define EDCHAR_INITIAL	0xFFFEU
static struct {
	unsigned short erase;
	unsigned short kill;
	unsigned short werase;
	unsigned short intr;
	unsigned short quit;
	unsigned short eof;
} edchars;

#define isched(x,e) ((unsigned short)(unsigned char)(x) == (e))
#define isedchar(x) (!((x) & ~0xFFU))
#define toedchar(x) ((unsigned short)KSH_ISVDIS(x, EDCHAR_DISABLED))

/* x_cf_glob() flags */
#define XCF_COMMAND	BIT(0)	/* Do command completion */
#define XCF_FILE	BIT(1)	/* Do file completion */
#define XCF_FULLPATH	BIT(2)	/* command completion: store full path */
#define XCF_COMMAND_FILE (XCF_COMMAND | XCF_FILE)
#define XCF_IS_COMMAND	BIT(3)	/* return flag: is command */
#define XCF_IS_NOSPACE	BIT(4)	/* return flag: do not append a space */
#define XCF_IS_HOMEDIR	BIT(5)	/* return flag: tilde needs slash */

static char editmode;
static int xx_cols;			/* for Emacs mode */
static int modified;			/* buffer has been "modified" */
static char *holdbufp;			/* place to hold last edit buffer */

/* 0=dumb 1=tmux (for now) */
static kby x_term_mode;

static void x_adjust(void);
static int x_getc(void);
static void x_putcf(int);
static void x_modified(void);
static void x_mode(Wahr);
static int x_do_comment(char *, ssize_t, ssize_t *);
static void x_print_expansions(int, char * const *, Wahr);
static int x_cf_glob(int *, const char *, int, int, int *, int *, char ***);
static size_t x_longest_prefix(int, char * const *);
static char *x_glob_hlp_add_qchar(char *);
static char *x_glob_hlp_tilde_and_rem_qchar(char *, Wahr);
static size_t x_basename(const char *, const char *);
static void x_free_words(int, char **);
static int x_escape(const char *, size_t, int (*)(const char *, size_t));
static ssize_t x_emacs(char *);
#ifdef DF
static void x_emacs_DF(const char *);
#endif
static void x_init_prompt(Wahr);
#if !MKSH_S_NOVI
static ssize_t x_vi(char *);
#endif
static void x_intr(int, int) MKSH_A_NORETURN;
static void x_clrtoeol(int, Wahr);

#define x_flush()	shf_flush(shl_out)
#if defined(MKSH_SMALL) && !defined(MKSH_SMALL_BUT_FAST)
#define x_putc(c)	x_putcf(c)
#else
#define x_putc(c)	shf_putc((c), shl_out)
#endif

static int path_order_cmp(const void *, const void *);
static void glob_table(const char *, XPtrV *, struct table *);
static void glob_path(int, const char *, XPtrV *, const char *);
static int x_file_glob(int *, char *, char ***);
static int x_command_glob(int, char *, char ***);
static int x_locate_word(const char *, int, int, int *, Wahr *);

static int x_e_getmbc(char *);

/* +++ generic editing functions +++ */

/*
 * read an edited command line
 */
char *
x_read(char *buf)
{
	ssize_t i;

	x_mode(Ja);
	modified = 1;
	if (Flag(FEMACS) || Flag(FGMACS))
		i = x_emacs(buf);
#if !MKSH_S_NOVI
	else if (Flag(FVI))
		i = x_vi(buf);
#endif
	else
		/* internal error */
		i = -1;
	editmode = 0;
	x_mode(Nee);
	if (i < 0) {
		/* pdksh had no error handling here, just return null */
		return (buf);
	}
	buf += i;
	return (buf);
}

/* tty I/O */

static int
x_getc(void)
{
#ifdef __OS2__
	return (_read_kbd(0, 1, 0));
#else
	char c;
	ssize_t n;

	while ((n = blocking_read(0, &c, 1)) < 0 && errno == EINTR)
		if (trap) {
			x_mode(Nee);
			runtraps(0);
#ifdef SIGWINCH
			if (got_winch) {
				change_winsz();
				if (x_cols != xx_cols && editmode == 1) {
					/* redraw line in Emacs mode */
					xx_cols = x_cols;
					x_init_prompt(Nee);
					x_adjust();
				}
			}
#endif
			x_mode(Ja);
		}
	return ((n == 1) ? (int)(unsigned char)c : -1);
#endif
}

static void
x_putcf(int c)
{
	shf_putc_i(c, shl_out);
}

/*********************************
 * Misc common code for vi/emacs *
 *********************************/

/*-
 * Handle the commenting/uncommenting of a line.
 * Returns:
 *	1 if a carriage return is indicated (comment added)
 *	0 if no return (comment removed)
 *	-1 if there is an error (not enough room for comment chars)
 * If successful, *lenp contains the new length. Note: cursor should be
 * moved to the start of the line after (un)commenting.
 */
static int
x_do_comment(char *buf, ssize_t bsize, ssize_t *lenp)
{
	ssize_t i, j, len = *lenp;

	if (len == 0)
		/* somewhat arbitrary - it's what AT&T ksh does */
		return (1);

	/* Already commented? */
	if (buf[0] == '#') {
		Wahr saw_nl = Nee;

		for (j = 0, i = 1; i < len; i++) {
			if (!saw_nl || buf[i] != '#')
				buf[j++] = buf[i];
			saw_nl = buf[i] == '\n';
		}
		*lenp = j;
		return (0);
	} else {
		int n = 1;

		/* See if there's room for the #s - 1 per \n */
		for (i = 0; i < len; i++)
			if (buf[i] == '\n')
				n++;
		if (len + n >= bsize)
			return (-1);
		/* Now add them... */
		for (i = len, j = len + n; --i >= 0; ) {
			if (buf[i] == '\n')
				buf[--j] = '#';
			buf[--j] = buf[i];
		}
		buf[0] = '#';
		*lenp += n;
		return (1);
	}
}

/****************************************************
 * Common file/command completion code for vi/emacs *
 ****************************************************/

static void
x_print_expansions(int nwords, char * const *words, Wahr is_command)
{
	int i;
	char **w;
	size_t prefix_len = 0;
	Wahr prefix_trim = Nee;
	struct columnise_opts co;
	struct shf S;

	/*
	 * Check if all matches are in the same directory (in this
	 * case, we want to omit the directory name)
	 */
	if (!is_command &&
	    (prefix_len = x_longest_prefix(nwords, words)) > 0) {
		/* Special case for 1 match (prefix is whole word) */
		if (nwords == 1)
			prefix_len = x_basename(words[0], NULL);
		/* Any (non-trailing) slashes in non-common word suffixes? */
		for (i = 0; i < nwords; i++)
			if (x_basename(words[i] + prefix_len, NULL) >
			    prefix_len)
				break;
		/* All in same directory? */
		if (i == nwords) {
			while (prefix_len > 0 &&
			    !mksh_cdirsep(words[0][prefix_len - 1]))
				prefix_len--;
			prefix_trim = Ja;
		}
	}
	/*
	 * Escape words, trimming prefix_len if needed
	 */
	if (!prefix_trim)
		prefix_len = 0;
	w = alloc2((kui)nwords + 1U, sizeof(char *), ATEMP);
	for (i = 0; i < nwords; ++i) {
		shf_sopen(NULL, 0, SHF_WR | SHF_DYNAMIC, &S);
		uprntmbs(words[i] + prefix_len, Nee, &S);
		w[i] = shf_sclose(&S);
	}
	w[nwords] = NULL;

	/*
	 * Enumerate expansions
	 */
	x_putc('\r');
	x_putc('\n');
	co.shf = shl_out;
	co.linesep = '\n';
	co.do_last = Ja;
	co.prefcol = Nee;
	pr_list(&co, w);
	x_free_words(nwords, w);
}

/*
 * Convert backslash-escaped string to QCHAR-escaped
 * string useful for globbing
 */
static char *
x_glob_hlp_add_qchar(char *cp)
{
	char ch, *dp;
	Wahr escaping = Nee;
	XString xs;
	size_t n;

	if (memchr(cp, QCHAR, (n = strlen(cp)))) {
		Xinit(xs, dp, n, ATEMP);
	} else {
		xs.len = n + 1;
		xs.areap = NULL; /* won’t be used */
		xs.beg = dp = cp;
		xs.end = xs.beg + xs.len;
	}

	while ((ch = *cp++)) {
		if (ch == '\\' && !escaping) {
			escaping = Ja;
			continue;
		}
		/* coverity[var_deref_model : SUPPRESS] */
		XcheckN(xs, dp, 2);
		if ((escaping && ctype(ch, C_EDGLB)) || ch == QCHAR)
			*dp++ = QCHAR;
		escaping = Nee;
		*dp++ = ch;
	}
	*dp = '\0';
	return (Xstring(xs, dp));
}

/*
 * Run tilde expansion on argument string, return the result
 * after unescaping; if the flag is set, the original string
 * is freed if changed and assumed backslash-escaped, if not
 * it is assumed QCHAR-escaped
 */
static char *
x_glob_hlp_tilde_and_rem_qchar(char *s, Wahr magic_flag)
{
	char ch, *cp, *dp;

	/*
	 * On the string, check whether we have a tilde expansion,
	 * and if so, discern "~foo/bar" and "~/baz" from "~blah";
	 * if we have a directory part (the former), try to expand
	 */
	if (*s == '~' && (cp = /* not sdirsep */ ucstrchr(s, '/')) != NULL) {
		/* ok, so split into "~foo"/"bar" or "~"/"baz" */
		*cp++ = 0;
		/* try to expand the tilde */
		if (!(dp = do_tilde(s + 1))) {
			/* nope, revert damage */
			*--cp = '/';
		} else {
			/* ok, expand and replace */
			strpathx(cp, dp, cp, 1);
			if (magic_flag)
				afree(s, ATEMP);
			s = cp;
		}
	}

	/* ... convert it from backslash-escaped via QCHAR-escaped... */
	if (magic_flag) {
		cp = x_glob_hlp_add_qchar(s);
		if (cp != s)
			afree(s, ATEMP);
		s = cp;
	}
	/* ... to unescaped, for comparison with the matches */
	cp = dp = s;

	while ((ch = *cp++)) {
		if (ch == QCHAR && !(ch = *cp++))
			break;
		*dp++ = ch;
	}
	*dp = '\0';

	return (s);
}

/**
 * Do file globbing:
 *	- does expansion, checks for no match, etc.
 *	- sets *wordsp to array of matching strings
 *	- returns number of matching strings
 */
static int
x_file_glob(int *flagsp, char *toglob, char ***wordsp)
{
	char **words, *cp, *qglob;
	int nwords;
	XPtrV w;
	struct source *s, *sold;

	/* remove all escaping backward slashes */
	qglob = x_glob_hlp_add_qchar(toglob);

	/*
	 * Convert "foo*" (toglob) to an array of strings (words)
	 */
	sold = source;
	s = pushs(SWSTR, ATEMP);
	s->start = s->str = qglob;
	source = s;
	if (yylex(ONEWORD | LQCHAR) != LWORD) {
		source = sold;
		kwarnf(KWF_INTERNAL | KWF_WARNING | KWF_ONEMSG | KWF_NOERRNO,
		    Tfg_badsubst);
		nwords = 0;
		goto out;
	}
	source = sold;
	afree(s, ATEMP);
	XPinit(w, 32);
	cp = yylval.cp;
	while (*cp == CHAR || *cp == QCHAR)
		cp += 2;
	nwords = DOGLOB | DOTILDE | DOMARKDIRS;
	if (*cp != EOS) {
		/* probably a $FOO expansion */
		*flagsp |= XCF_IS_NOSPACE;
		/* this always results in at most one match */
		nwords = 0;
	}
	expand(yylval.cp, &w, nwords);
	XPput(w, NULL);
	nwords = 0;
	while (XPptrv(w)[nwords])
		++nwords;
	/* XPclose(w) except for nwords plus a trailing NULL for pr_list */
	words = aresize2(XPptrv(w), (kui)nwords + 1U, sizeof(void *), ATEMP);

	if (nwords == 1) {
		struct stat statb;

		/* Expand any tilde and drop all QCHAR for comparison */
		qglob = x_glob_hlp_tilde_and_rem_qchar(qglob, Nee);

		/*
		 * Check if globbing failed (returned glob pattern),
		 * but be careful (e.g. toglob == "ab*" when the file
		 * "ab*" exists is not an error).
		 * Also, check for empty result - happens if we tried
		 * to glob something which evaluated to an empty
		 * string (e.g., "$FOO" when there is no FOO, etc).
		 */
		if ((strcmp(words[0], qglob) == 0 &&
		    stat(words[0], &statb) < 0) ||
		    words[0][0] == '\0') {
			x_free_words(nwords, words);
			words = NULL;
			nwords = 0;
		}
	}

	if ((*wordsp = nwords ? words : NULL) == NULL && words != NULL)
		x_free_words(nwords, words);

 out:
	if (qglob != toglob)
		afree(qglob, ATEMP);
	return (nwords);
}

/* Data structure used in x_command_glob() */
struct path_order_info {
	char *word;
	size_t base;
	size_t path_order;
};

/* Compare routine used in x_command_glob() */
static int
path_order_cmp(const void *aa, const void *bb)
{
	const struct path_order_info *a = (const struct path_order_info *)aa;
	const struct path_order_info *b = (const struct path_order_info *)bb;
	int t;

	if ((t = ascstrcmp(a->word + a->base, b->word + b->base)))
		return (t);
	if (a->path_order > b->path_order)
		return (1);
	if (a->path_order < b->path_order)
		return (-1);
	return (0);
}

static int
x_command_glob(int flags, char *toglob, char ***wordsp)
{
	char *pat, *fpath;
	size_t nwords;
	XPtrV w;
	struct block *l;

	/* Convert "foo*" (toglob) to a pattern for future use */
	pat = evalstr(toglob, DOPAT | DOTILDE);

	XPinit(w, 32);

	glob_table(pat, &w, &keywords);
	glob_table(pat, &w, &aliases);
	glob_table(pat, &w, &builtins);
	for (l = e->loc; l; l = l->next)
		glob_table(pat, &w, &l->funs);

	glob_path(flags, pat, &w, path);
	if ((fpath = str_val(global(TFPATH))) != null)
		glob_path(flags, pat, &w, fpath);

	nwords = XPsize(w);

	if (!nwords) {
		*wordsp = NULL;
		XPfree(w);
		return (0);
	}
	/* Sort entries */
	if (flags & XCF_FULLPATH) {
		/* Sort by basename, then path order */
		struct path_order_info *info, *last_info = NULL;
		char **words = (char **)XPptrv(w);
		size_t i, path_order = 0;

		info = (struct path_order_info *)
		    alloc2(nwords, sizeof(struct path_order_info), ATEMP);
		for (i = 0; i < nwords; i++) {
			info[i].word = words[i];
			info[i].base = x_basename(words[i], NULL);
			if (!last_info || info[i].base != last_info->base ||
			    strncmp(words[i], last_info->word, info[i].base) != 0) {
				last_info = &info[i];
				path_order++;
			}
			info[i].path_order = path_order;
		}
		qsort(info, nwords, sizeof(struct path_order_info),
		    path_order_cmp);
		for (i = 0; i < nwords; i++)
			words[i] = info[i].word;
		afree(info, ATEMP);
	} else {
		/* Sort and remove duplicate entries */
		char **words = (char **)XPptrv(w);
		size_t i, j;

		qsort(words, nwords, sizeof(void *), ascpstrcmp);
		for (i = j = 0; i < nwords - 1; i++) {
			if (strcmp(words[i], words[i + 1]))
				words[j++] = words[i];
			else
				afree(words[i], ATEMP);
		}
		words[j++] = words[i];
		w.len = nwords = j;
	}

	XPput(w, NULL);
	*wordsp = (char **)XPclose(w);

	return (nwords);
}

#define IS_WORDC(c)	(!ctype(c, C_EDNWC))

static int
x_locate_word(const char *buf, int buflen, int pos, int *startp,
    Wahr *is_commandp)
{
	int start, end;

	/* Bad call? Probably should report error */
	if (pos < 0 || pos > buflen) {
		*startp = pos;
		*is_commandp = Nee;
		return (0);
	}
	/* The case where pos == buflen happens to take care of itself... */

	start = pos;
	/*
	 * Keep going backwards to start of word (has effect of allowing
	 * one blank after the end of a word)
	 */
	for (; (start > 0 && IS_WORDC(buf[start - 1])) ||
	    (start > 1 && buf[start - 2] == '\\'); start--)
		;
	/* Go forwards to end of word */
	for (end = start; end < buflen && IS_WORDC(buf[end]); end++) {
		if (buf[end] == '\\' && (end + 1) < buflen)
			end++;
	}

	if (is_commandp) {
		Wahr iscmd;
		int p = start - 1;

		/* Figure out if this is a command */
		while (p >= 0 && ctype(buf[p], C_SPACE))
			p--;
		iscmd = p < 0 || ctype(buf[p], C_EDCMD);
		if (iscmd) {
			/*
			 * If command has a /, path, etc. is not searched;
			 * only current directory is searched which is just
			 * like file globbing.
			 */
			for (p = start; p < end; p++)
				if (mksh_cdirsep(buf[p]))
					break;
			iscmd = p == end;
		}
		*is_commandp = iscmd;
	}
	*startp = start;

	return (end - start);
}

static int
x_cf_glob(int *flagsp, const char *buf, int buflen, int pos, int *startp,
    int *endp, char ***wordsp)
{
	int len, nwords = 0;
	char **words = NULL;
	Wahr is_command;

	len = x_locate_word(buf, buflen, pos, startp, &is_command);
	if (!((*flagsp) & XCF_COMMAND))
		is_command = Nee;
	/*
	 * Don't do command globing on zero length strings - it takes too
	 * long and isn't very useful. File globs are more likely to be
	 * useful, so allow these.
	 */
	if (len == 0 && is_command)
		return (0);

	if (len >= 0) {
		char *toglob, *s;

		/*
		 * Given a string, copy it and possibly add a '*' to the end.
		 */

		strndupx(toglob, buf + *startp, len + /* the '*' */ 1, ATEMP);
		toglob[len] = '\0';

		/*
		 * If the pathname contains a wildcard (an unquoted '*',
		 * '?', or '[') or an extglob, then it is globbed based
		 * on that value (i.e., without the appended '*'). Same
		 * for parameter substitutions (as in “cat $HOME/.ss↹”)
		 * without appending a trailing space (LP: #710539), as
		 * well as for “~foo” (but not “~foo/”).
		 */
		for (s = toglob; *s; s++) {
			if (*s == '\\' && s[1])
				s++;
			else if (ctype(*s, C_QUEST | C_DOLAR) ||
			    ord(*s) == ORD('*') || ord(*s) == ORD('[') ||
			    /* ?() *() +() @() !() but two already checked */
			    (ord(s[1]) == ORD('(' /*)*/) &&
			    (ord(*s) == ORD('+') || ord(*s) == ORD('@') ||
			    ord(*s) == ORD('!')))) {
				/*
				 * just expand based on the extglob
				 * or parameter
				 */
				goto dont_add_glob;
			}
		}

		if (*toglob == '~' && /* not vdirsep */ !vstrchr(toglob, '/')) {
			/* neither for '~foo' (but '~foo/bar') */
			*flagsp |= XCF_IS_HOMEDIR;
			goto dont_add_glob;
		}

		/* append a glob */
		toglob[len] = '*';
		toglob[len + 1] = '\0';
 dont_add_glob:
		/*
		 * Expand (glob) it now.
		 */

		nwords = is_command ?
		    x_command_glob(*flagsp, toglob, &words) :
		    x_file_glob(flagsp, toglob, &words);
		afree(toglob, ATEMP);
	}
	if (nwords == 0) {
		*wordsp = NULL;
		return (0);
	}
	if (is_command)
		*flagsp |= XCF_IS_COMMAND;
	*wordsp = words;
	*endp = *startp + len;

	return (nwords);
}

/*
 * Find longest common prefix
 */
static size_t
x_longest_prefix(int nwords, char * const * words)
{
	int i;
	size_t j, prefix_len;
	char *p;

	if (nwords <= 0)
		return (0);

	prefix_len = strlen(words[0]);
	for (i = 1; i < nwords; i++)
		for (j = 0, p = words[i]; j < prefix_len; j++)
			if (p[j] != words[0][j]) {
				prefix_len = j;
				break;
			}
	/* false for nwords==1 as 0 = words[0][prefix_len] then */
	if (UTFMODE && prefix_len && (rtt2asc(words[0][prefix_len]) & 0xC0) == 0x80)
		while (prefix_len && (rtt2asc(words[0][prefix_len]) & 0xC0) != 0xC0)
			--prefix_len;
	return (prefix_len);
}

static void
x_free_words(int nwords, char **words)
{
	while (nwords)
		afree(words[--nwords], ATEMP);
	afree(words, ATEMP);
}

/*-
 * Return the offset of the basename of string s (which ends at se - need not
 * be null terminated). Trailing slashes are ignored. If s is just a slash,
 * then the offset is 0 (actually, length - 1).
 *	s		Return
 *	/etc		1
 *	/etc/		1
 *	/etc//		1
 *	/etc/fo		5
 *	foo		0
 *	///		2
 *			0
 */
static size_t
x_basename(const char *s, const char *se)
{
	const char *p;

	if (se == NULL)
		se = strnul(s);
	if (s == se)
		return (0);

	/* skip trailing directory separators */
	p = se - 1;
	while (p > s && mksh_cdirsep(*p))
		--p;
	/* drop last component */
	while (p > s && !mksh_cdirsep(*p))
		--p;
	if (mksh_cdirsep(*p) && p + 1 < se)
		++p;

	return (p - s);
}

/*
 * Apply pattern matching to a table: all table entries that match a pattern
 * are added to wp.
 */
static void
glob_table(const char *pat, XPtrV *wp, struct table *tp)
{
	struct tstate ts;
	struct tbl *te;

	ktwalk(&ts, tp);
	while ((te = ktnext(&ts)))
		if (gmatchx(te->name, pat, Nee)) {
			char *cp;

			strdupx(cp, te->name, ATEMP);
			XPput(*wp, cp);
		}
}

static void
glob_path(int flags, const char *pat, XPtrV *wp, const char *lpath)
{
	const char *sp = lpath, *p;
	char *xp, **words;
	size_t pathlen, patlen, oldsize, newsize, i, j;
	XString xs;

	patlen = strlen(pat);
	checkoktoadd(patlen, 129 + X_EXTRA);
	++patlen;
	Xinit(xs, xp, patlen + 128, ATEMP);
	while (sp) {
		xp = Xstring(xs, xp);
		if (!(p = cstrchr(sp, MKSH_PATHSEPC)))
			p = strnul(sp);
		pathlen = p - sp;
		if (pathlen) {
			/*
			 * Copy sp into xp, stuffing any MAGIC characters
			 * on the way
			 */
			const char *s = sp;

			XcheckN(xs, xp, pathlen * 2);
			while (s < p) {
				if (ISMAGIC(*s))
					*xp++ = MAGIC;
				*xp++ = *s++;
			}
			*xp++ = '/';
			pathlen++;
		}
		sp = p;
		XcheckN(xs, xp, patlen);
		memcpy(xp, pat, patlen);

		oldsize = XPsize(*wp);
		/* mark dirs */
		glob_str(Xstring(xs, xp), wp, Ja);
		newsize = XPsize(*wp);

		/* Check that each match is executable... */
		words = (char **)XPptrv(*wp);
		for (i = j = oldsize; i < newsize; i++) {
			if (ksh_access(words[i], X_OK) == 0) {
				words[j] = words[i];
				if (!(flags & XCF_FULLPATH))
					memmove(words[j], words[j] + pathlen,
					    strlen(words[j] + pathlen) + 1);
				j++;
			} else
				afree(words[i], ATEMP);
		}
		wp->len = j;

		if (!*sp++)
			break;
	}
	Xfree(xs, xp);
}

/*
 * if argument string contains any special characters, they will
 * be escaped and the result will be put into edit buffer by
 * keybinding-specific function
 */
static int
x_escape(const char *s, size_t len, int (*putbuf_func)(const char *, size_t))
{
	size_t add = 0, wlen = len;
	int rval = 0;

	while (wlen - add > 0)
		if (ctype(s[add], C_IFS | C_EDQ)) {
			if (putbuf_func(s, add) != 0) {
				rval = -1;
				break;
			}
			putbuf_func(s[add] == '\n' ? "'" : "\\", 1);
			putbuf_func(&s[add], 1);
			if (s[add] == '\n')
				putbuf_func("'", 1);

			add++;
			wlen -= add;
			s += add;
			add = 0;
		} else
			++add;
	if (wlen > 0 && rval == 0)
		rval = putbuf_func(s, wlen);

	return (rval);
}

/* +++ emacs editing mode +++ */

/*-
 * The input buffer "buf" is pointed to by "xbuf" and its end is
 * pointed to by "xend".  The current position in "xbuf" and end of
 * the edit line are pointed to by "xcp" and "xep" respectively.
 * "xbp" points to the start of a display window within "xbuf", and
 * "xlp" points to the last visible character on screen, if valid;
 * "xdp" points to the end of x_displen, adding one-column bytes if
 * necessary when the input line is shorter.
 *
 * [A] starting position
 *
 *      buf
 * |<--------- $COLUMNS -------->|
 *      |<---- x_displen ------->|
 *  PS1 |
 *      +=====+=========+........+.............................+
 *      |\     \        |\        \                             \
 *   xbuf xbp   xcp   xlp xep      xdp                           xend
 *
 * [B] larger input line
 *
 *      buf
 * |<--------- $COLUMNS -------->|
 *      |<---- x_displen ------->|
 *  PS1 |
 *      +===========+============+---------------------+.......+
 *      |\          \             \                     \       \
 *   xbuf xbp        xcp           xlp=xdp               xep     xend
 *
 * [C] scrolled
 *
 *      buf
 *      |       |<--------- $COLUMNS -------->|
 *      |       |<--------- x_displen ------->|
 *      |
 *      +-------+==============+==============+--------+.......+
 *      |        \              \              \        \       \
 *   xbuf         xbp            xcp            xlp=xdp  xep     xend
 *
 * In the above -------- represents the current edit line while
 * ===== represents that portion which is visible on the screen;
 * ....... is unused space in buf. Note that initially xbp == xbuf
 * and PS1 is displayed. PS1 uses some of the screen width and thus
 * "x_displen" is less than $COLUMNS.
 *
 * Any time that "xcp" moves outside the region bounded by "xbp"
 * and "xbp" + "x_displen", the function x_adjust() is called to
 * relocate "xbp" appropriately and redraw the line.
 *
 * Excessive I/O is avoided where possible.  x_goto() for instance
 * calculates whether the destination is outside the visible
 * region, and if so simply adjusts "xcp" and calls x_adjust()
 * directly.  Normally though x_adjust() is called from x_putc().
 */

static	Area	aedit;
#define	AEDIT	&aedit		/* area for kill ring and macro defns */

/* values returned by keyboard functions */
#define	KSTD	0
#define	KEOL	1		/* ^M, ^J */
#define	KINTR	2		/* ^G, ^C */

struct x_ftab {
	int (*xf_func)(int c);
	const char *xf_name;
	short xf_flags;
};

struct x_defbindings {
	unsigned char xdb_func;	/* XFUNC_* */
	unsigned char xdb_tab;
	unsigned char xdb_char;
};

#define XF_ARG		1	/* command takes number prefix */
#define	XF_NOBIND	2	/* not allowed to bind to function */
#define	XF_PREFIX	4	/* function sets prefix */

/*-
 * Arguments for do_complete()
 * 0 = enumerate	M-=	complete as much as possible and then list
 * 1 = complete		M-Esc
 * 2 = list		M-?
 */
typedef enum {
	CT_LIST,	/* list the possible completions */
	CT_COMPLETE,	/* complete to longest prefix */
	CT_COMPLIST	/* complete and then list (if non-exact) */
} Comp_type;

/*
 * The following are used for my horizontal scrolling stuff
 */
static char *xbuf;		/* beg input buffer */
static char *xend;		/* end input buffer */
static char *xcp;		/* current position */
static char *xep;		/* current end */
static char *xbp;		/* start of visible portion of input buffer */
static char *xlp;		/* last char visible on screen */
static char *xdp;		/* xbuf + x_displen except multibyte-aware */
static Wahr x_adj_ok;
/*
 * we use x_adj_done so that functions can tell
 * whether x_adjust() has been called while they are active.
 */
static kby x_adj_done;		/* is incremented by x_adjust() */

static int x_displen;
static int x_arg;		/* general purpose arg */
static Wahr x_arg_defaulted;	/* x_arg not explicitly set; defaulted to 1 */

/* indicates both xlp and xdp are valid (x_goto needs the latter) */
static Wahr xlp_valid;		/* lastvis pointer was recalculated */

static char **x_histp;		/* history position */
static int x_nextcmd;		/* for newline-and-next */
static char **x_histncp;	/* saved x_histp for " */
static char **x_histmcp;	/* saved x_histp for " */
static char *xmp;		/* mark pointer */
static unsigned char x_last_command;
static kby x_curprefix;
static kby *x_btab;		/* bitmap of keys bound by the user */
static kby *x_ktab;		/* key definitions */
#ifndef MKSH_SMALL
static char **x_mtab;		/* macro definitions */
static char *macroptr;		/* bind key macro active? */
#endif
#define KILLSIZE	20
static char *killstack[KILLSIZE];
static int killsp, killtp;
#if !MKSH_S_NOVI
static int winwidth;		/* width of window */
static char *wbuf[2];		/* window buffers */
static int wbuf_len;		/* length of window buffers (x_cols - 3) */
static int win;			/* window buffer in use */
static char morec;		/* more character at right of window */
static int holdlen;		/* length of holdbuf */
#endif
static int pwidth;		/* width of prompt */
static int prompt_trunc;	/* how much of prompt to truncate or -1 */
static int x_col;		/* current column on line */

/* normal, prefix1, prefix2, IBM PC, ^V (literal) */
#define X_NTABS			4U
#define X_NOTAB			0xFFU
/* array indicēs; x_btab is inlined, bitmap at only two places */
#define X_xTABidx(pfx,key)	((KBI(pfx) << 8) | KBI(key))
#define X_KTAB(pfx,key)		x_ktab[X_xTABidx((pfx), (key))]
#define X_MTAB(pfx,key)		x_mtab[X_xTABidx((pfx), (key))]

static int x_ins(const void *);
static void x_delete(size_t, Wahr);
static void x_bword(kui, Wahr);
static void x_fword(kui, Wahr);
static void x_goto(char *);
static void x_bs3(char **);
static void x_uescs(char *);
static void x_uescmb(char **);
static void x_load_hist(char **);
static int x_search(const char *, int, int);
#ifndef MKSH_SMALL
static int x_search_dir(int);
#endif
static int x_match(const char *, const char *);
static void x_redraw(int);
static void x_push(size_t);
static void x_bind_showone(kui, kui);
static void x_e_ungetc(int);
static int x_e_getc(void);
static void x_e_putb(int);
static void x_e_puts(const char *);
#ifndef MKSH_SMALL
static int x_fold_case(int, kui);
#endif
static char *x_lastcp(void);
static void x_lastpos(void);
static void do_complete(int, Comp_type);
static size_t x_nb2nc(size_t);

static int unget_char = -1;

static int x_do_ins(const char *, size_t);
static void bind_if_not_bound(int, int, int);

enum emacs_funcs {
#define EMACSFN_ENUMS
#include "emacsfn.h"
	XFUNC_MAX
};

#define EMACSFN_DEFNS
#include "emacsfn.h"

static const struct x_ftab x_ftab[] = {
#define EMACSFN_ITEMS
#include "emacsfn.h"
};

static struct x_defbindings const x_defbindings[] = {
	{ XFUNC_del_back,		0,  CTRL_QM	},
	{ XFUNC_del_bword,		1,  CTRL_QM	},
	{ XFUNC_eot_del,		0,  CTRL_D	},
	{ XFUNC_del_back,		0,  CTRL_H	},
	{ XFUNC_del_bword,		1,  CTRL_H	},
	{ XFUNC_del_bword,		1,	'h'	},
	{ XFUNC_mv_bword,		1,	'b'	},
	{ XFUNC_mv_fword,		1,	'f'	},
	{ XFUNC_del_fword,		1,	'd'	},
#ifndef MKSH_SMALL
	{ XFUNC_del_bbigword,		1,	'H'	},
	{ XFUNC_mv_bbigword,		1,	'B'	},
	{ XFUNC_mv_fbigword,		1,	'F'	},
	{ XFUNC_del_fbigword,		1,	'D'	},
#endif
	{ XFUNC_mv_back,		0,  CTRL_B	},
	{ XFUNC_mv_forw,		0,  CTRL_F	},
	{ XFUNC_search_char_forw,	0,  CTRL_BC	},
	{ XFUNC_search_char_back,	1,  CTRL_BC	},
	{ XFUNC_newline,		0,  CTRL_M	},
	{ XFUNC_newline,		0,  CTRL_J	},
	{ XFUNC_end_of_text,		0,  CTRL_US	},
	{ XFUNC_abort,			0,  CTRL_G	},
	{ XFUNC_prev_com,		0,  CTRL_P	},
	{ XFUNC_next_com,		0,  CTRL_N	},
	{ XFUNC_nl_next_com,		0,  CTRL_O	},
	{ XFUNC_search_hist,		0,  CTRL_R	},
	{ XFUNC_beg_hist,		1,	'<'	},
	{ XFUNC_end_hist,		1,	'>'	},
	{ XFUNC_goto_hist,		1,	'g'	},
	{ XFUNC_mv_end,			0,  CTRL_E	},
	{ XFUNC_mv_beg,			0,  CTRL_A	},
	{ XFUNC_draw_line,		0,  CTRL_L	},
	{ XFUNC_cls,			1,  CTRL_L	},
	{ XFUNC_meta1,			0,  CTRL_BO	},
	{ XFUNC_meta2,			0,  CTRL_X	},
	{ XFUNC_kill,			0,  CTRL_K	},
	{ XFUNC_yank,			0,  CTRL_Y	},
	{ XFUNC_meta_yank,		1,	'y'	},
	{ XFUNC_literal,		0,  CTRL_CA	},
	{ XFUNC_comment,		1,	'#'	},
	{ XFUNC_transpose,		0,  CTRL_T	},
	{ XFUNC_complete,		1,  CTRL_BO	},
	{ XFUNC_comp_list,		0,  CTRL_I	},
	{ XFUNC_comp_list,		1,	'='	},
	{ XFUNC_enumerate,		1,	'?'	},
	{ XFUNC_expand,			1,	'*'	},
	{ XFUNC_comp_file,		1,  CTRL_X	},
	{ XFUNC_comp_comm,		2,  CTRL_BO	},
	{ XFUNC_list_comm,		2,	'?'	},
	{ XFUNC_list_file,		2,  CTRL_Y	},
	{ XFUNC_set_mark,		1,	' '	},
	{ XFUNC_kill_region,		1,  CTRL_W	},
	{ XFUNC_xchg_point_mark,	2,  CTRL_X	},
	{ XFUNC_literal,		0,  CTRL_V	},
	{ XFUNC_version,		1,  CTRL_V	},
	{ XFUNC_prev_histword,		1,	'.'	},
	{ XFUNC_prev_histword,		1,	'_'	},
	{ XFUNC_set_arg,		1,	'0'	},
	{ XFUNC_set_arg,		1,	'1'	},
	{ XFUNC_set_arg,		1,	'2'	},
	{ XFUNC_set_arg,		1,	'3'	},
	{ XFUNC_set_arg,		1,	'4'	},
	{ XFUNC_set_arg,		1,	'5'	},
	{ XFUNC_set_arg,		1,	'6'	},
	{ XFUNC_set_arg,		1,	'7'	},
	{ XFUNC_set_arg,		1,	'8'	},
	{ XFUNC_set_arg,		1,	'9'	},
#ifndef MKSH_SMALL
	{ XFUNC_foldb_upper,		1,	'U'	},
	{ XFUNC_fold_upper,		1,	'u'	},
	{ XFUNC_foldb_lower,		1,	'L'	},
	{ XFUNC_fold_lower,		1,	'l'	},
	{ XFUNC_foldb_capitalise,	1,	'C'	},
	{ XFUNC_fold_capitalise,	1,	'c'	},
#endif
	/*
	 * These for ANSI arrow keys: arguablely shouldn't be here by
	 * default, but its simpler/faster/smaller than using termcap
	 * entries.
	 */
	{ XFUNC_meta2,			1,	'['	},
	{ XFUNC_meta2,			1,	'O'	},
	{ XFUNC_prev_com,		2,	'A'	},
	{ XFUNC_next_com,		2,	'B'	},
	{ XFUNC_mv_forw,		2,	'C'	},
	{ XFUNC_mv_back,		2,	'D'	},
#ifndef MKSH_SMALL
	{ XFUNC_vt_hack,		2,	'1'	},
	{ XFUNC_mv_beg | 0x80,		2,	'7'	},
	{ XFUNC_mv_beg,			2,	'H'	},
	{ XFUNC_mv_end | 0x80,		2,	'4'	},
	{ XFUNC_mv_end | 0x80,		2,	'8'	},
	{ XFUNC_mv_end,			2,	'F'	},
	{ XFUNC_del_char | 0x80,	2,	'3'	},
	{ XFUNC_del_char,		2,	'P'	},
	{ XFUNC_search_hist_up | 0x80,	2,	'5'	},
	{ XFUNC_search_hist_dn | 0x80,	2,	'6'	},
#endif
	/* PC scancodes */
#if !defined(MKSH_SMALL) || defined(__OS2__)
	{ XFUNC_meta3,			0,	0	},
	{ XFUNC_mv_beg,			3,	71	},
	{ XFUNC_prev_com,		3,	72	},
#ifndef MKSH_SMALL
	{ XFUNC_search_hist_up,		3,	73	},
#endif
	{ XFUNC_mv_back,		3,	75	},
	{ XFUNC_mv_forw,		3,	77	},
	{ XFUNC_mv_end,			3,	79	},
	{ XFUNC_next_com,		3,	80	},
#ifndef MKSH_SMALL
	{ XFUNC_search_hist_dn,		3,	81	},
#endif
	{ XFUNC_del_char,		3,	83	},
#endif
#ifndef MKSH_SMALL
	/* more non-standard ones */
	{ XFUNC_eval_region,		1,  CTRL_E	},
	{ XFUNC_quote_region,		1,	'Q'	},
	{ XFUNC_edit_line,		2,	'e'	}
#endif
};

static size_t
x_nb2nc(size_t nb)
{
	char *cp;
	size_t nc = 0;

	for (cp = xcp; cp < (xcp + nb); ++nc)
		cp += ez_mbtoc(NULL, cp);
	return (nc);
}

static void
x_modified(void)
{
	if (!modified) {
		x_histmcp = x_histp;
		x_histp = histptr + 1;
		modified = 1;
	}
}

#ifdef MKSH_SMALL
#define XFUNC_VALUE(f) (f)
#else
#define XFUNC_VALUE(f) (f & 0x7FU)
#endif

static int
x_e_getmbc(char *buf)
{
	int c, pos;
	kby f;

	if ((c = x_e_getc()) == -1)
		return (-1);
	buf[0] = c;
	if (!UTFMODE) {
		buf[1] = '\0';
		return (1);
	}

	pos = 1;
	f = rtt2asc(c);
	if (f >= 0xC2U && f < 0xF0U) {
		if ((c = x_e_getc()) == -1)
			return (-1);
		if ((rtt2asc(c) & 0xC0U) != 0x80U) {
			x_e_ungetc(c);
			goto out;
		}
		buf[pos++] = c;
		if (f >= 0xE0U) {
			if ((c = x_e_getc()) == -1)
				return (-1);
			/* XXX x_e_ungetc is one-octet only */
			if ((rtt2asc(c) & 0xC0U) != 0x80U)
				x_e_ungetc(c);
			else
				buf[pos++] = c;
		}
	}
 out:
	buf[pos] = '\0';
	return (pos);
}

/*
 * minimum required space to work with on a line - if the prompt
 * leaves less space than this on a line, the prompt is truncated
 */
#define MIN_EDIT_SPACE	7

static void
x_init_prompt(Wahr doprint)
{
	prompt_trunc = pprompt(prompt, doprint ? 0 : -1);
	pwidth = prompt_trunc % x_cols;
	prompt_trunc -= pwidth;
	if ((mksh_uari_t)pwidth > ((mksh_uari_t)x_cols - 3 - MIN_EDIT_SPACE)) {
		/* force newline after prompt */
		prompt_trunc = -1;
		pwidth = 0;
		if (doprint)
			x_e_putb('\n');
	}
}

static ssize_t
x_emacs(char *buf)
{
	int c, i;
	unsigned char f;

	xbp = xbuf = buf;
	xend = buf + LINE;
	xlp = xcp = xep = buf;
	*xcp = 0;
	xmp = NULL;
	x_curprefix = 0;
	x_histmcp = x_histp = histptr + 1;
	x_last_command = XFUNC_error;

	xx_cols = x_cols;
	x_adj_ok = Nee;
	x_init_prompt(Ja);
	x_displen = xx_cols - 2 - (x_col = pwidth);
	xdp = xbp + x_displen;
	xlp_valid = Ja;
	x_adj_done = 0;
	x_adj_ok = Ja;

	x_histncp = NULL;
	if (x_nextcmd >= 0) {
		int off = source->line - x_nextcmd;
		if (histptr - history >= off) {
			x_load_hist(histptr - off);
			x_histncp = x_histp;
		}
		x_nextcmd = -1;
	}
	editmode = 1;
	while (/* CONSTCOND */ 1) {
		x_flush();
		if ((c = x_e_getc()) < 0)
			return (-1);

		f = x_curprefix == X_NOTAB ? XFUNC_insert :
		    X_KTAB(x_curprefix, c);
#ifndef MKSH_SMALL
		if (f & 0x80) {
			f &= 0x7F;
			if ((i = x_e_getc()) != '~')
				x_e_ungetc(i);
		}

		/* avoid bind key macro recursion */
		if (macroptr && f == XFUNC_ins_string)
			f = XFUNC_insert;
#endif

		if (!(x_ftab[f].xf_flags & XF_PREFIX) &&
		    x_last_command != XFUNC_set_arg) {
			x_arg = 1;
			x_arg_defaulted = Ja;
		}
		i = (int)X_xTABidx(x_curprefix, c);
		x_curprefix = 0;
		switch ((*x_ftab[f].xf_func)(i)) {
		case KSTD:
			if (!(x_ftab[f].xf_flags & XF_PREFIX))
				x_last_command = f;
			break;
		case KEOL:
			return (xep - xbuf);
		case KINTR:
			/* special case for interrupt */
			x_intr(SIGINT, c);
		}
		/* ad-hoc hack for fixing the cursor position */
		x_goto(xcp);
	}
}

static int
x_insert(int c)
{
	static kby str[5], state;
	static int save_arg;
	kby b;

	/*
	 * Should allow tab and control chars.
	 */
	if (c == 0) {
		x_e_putb(KSH_BEL);
		state = 0;
		return (KSTD);
	}
	if (!UTFMODE) {
 sbc:
		str[0] = c;
		str[1] = '\0';
 mbc:
		while (x_arg--)
			x_ins(str);
		state = 0;
		return (KSTD);
	}
	b = rtt2asc(c);
	if (state) {
		/* this a continuation octet? */
		if ((b & 0xC0U) == 0x80U) {
			/* states: 1, 5, 6 */
			str[state & 3] = c;
			++state;
			/* states: 2, 6, 7 */
			if (state == 6)
				/* another octet needed */
				return (KSTD);
			/* finishing up this multibyte character */
			str[state & 3] = '\0';
			x_arg = save_arg;
			goto mbc;
		}
		/* flush invalid multibyte octets */
		str[state & 3] = '\0';
		while (save_arg--)
			x_ins(str);
		/* begin a new multibyte character */
	}
	if (b < 0xC2U || b >= 0xF0)
		goto sbc;
	state = b < 0xE0U ? 1 : 5;
	str[0] = c;
	save_arg = x_arg;
	return (KSTD);
}

#ifndef MKSH_SMALL
static int
x_ins_string(int c)
{
	macroptr = x_mtab[KUI(c)];
	/*
	 * we no longer need to bother checking if macroptr is
	 * not NULL but first char is NUL; x_e_getc() does it
	 */
	return (KSTD);
}
#endif

static int
x_do_ins(const char *cp, size_t len)
{
	if (xep + len >= xend) {
		x_e_putb(KSH_BEL);
		return (-1);
	}
	memmove(xcp + len, xcp, xep - xcp + 1);
	memmove(xcp, cp, len);
	xcp += len;
	xep += len;
	x_modified();
	return (0);
}

static int
x_ins(const void *s)
{
	char *cp = xcp;
	kby adj = x_adj_done;

	if (x_do_ins(s, strlen(s)) < 0)
		return (-1);
	/*
	 * x_uescs() may result in a call to x_adjust()
	 * we want xcp to reflect the new position
	 */
	xlp_valid = Nee;
	x_lastcp();
	x_adj_ok = isWahr(xcp >= xlp);
	x_uescs(cp);
	if (adj == x_adj_done)
		/* x_adjust() has not been called */
		x_lastpos();
	x_adj_ok = Ja;
	return (0);
}

static int
x_del_back(int c MKSH_A_UNUSED)
{
	ssize_t i = 0;

	if (xcp == xbuf) {
		x_e_putb(KSH_BEL);
		return (KSTD);
	}
	do {
		x_goto(xcp - 1);
	} while ((++i < x_arg) && (xcp != xbuf));
	x_delete(i, Nee);
	return (KSTD);
}

static int
x_del_char(int c MKSH_A_UNUSED)
{
	char *cp, *cp2;
	size_t i = 0;

	cp = xcp;
	while (i < (size_t)x_arg) {
		cp2 = cp + ez_mbtoc(NULL, cp);
		if (cp2 > xep)
			break;
		cp = cp2;
		i++;
	}

	if (!i) {
		x_e_putb(KSH_BEL);
		return (KSTD);
	}
	x_delete(i, Nee);
	return (KSTD);
}

/* Delete nc chars to the right of the cursor (including cursor position) */
static void
x_delete(size_t nc, Wahr push)
{
	size_t i, nb, nw;
	char *cp;

	if (nc == 0)
		return;

	nw = 0;
	cp = xcp;
	for (i = 0; i < nc; ++i) {
		char *cp2;
		int j;

		j = uwidthmbT(cp, &cp2);
		if (cp2 > xep)
			break;
		cp = cp2;
		nw += j;
	}
	nb = cp - xcp;
	/* nc = i; */

	if (xmp != NULL && xmp > xcp) {
		if (xcp + nb > xmp)
			xmp = xcp;
		else
			xmp -= nb;
	}
	/*
	 * This lets us yank a word we have deleted.
	 */
	if (push)
		x_push(nb);

	xep -= nb;
	/* Copies the NUL */
	memmove(xcp, xcp + nb, xep - xcp + 1);
	/* don't redraw */
	x_adj_ok = Nee;
	xlp_valid = Nee;
	x_uescs(xcp);
	/*
	 * if we are already filling the line,
	 * there is no need to ' ', '\b'.
	 * But if we must, make sure we do the minimum.
	 */
	if ((i = xx_cols - 2 - x_col) > 0 || xep - xlp == 0) {
		nw = i = (nw < i) ? nw : i;
		while (i--)
			x_e_putb(' ');
		if (x_col == xx_cols - 2) {
			x_e_putb((xep > xlp) ? '>' : (xbp > xbuf) ? '<' : ' ');
			++nw;
		}
		while (nw--)
			x_e_putb('\b');
	}
	/*x_goto(xcp);*/
	x_adj_ok = Ja;
	xlp_valid = Nee;
	x_lastpos();
	x_modified();
	return;
}

static int
x_del_bword(int c MKSH_A_UNUSED)
{
	x_bword(C_MFS, Ja);
	return (KSTD);
}

static int
x_mv_bword(int c MKSH_A_UNUSED)
{
	x_bword(C_MFS, Nee);
	return (KSTD);
}

static int
x_mv_fword(int c MKSH_A_UNUSED)
{
	x_fword(C_MFS, Nee);
	return (KSTD);
}

static int
x_del_fword(int c MKSH_A_UNUSED)
{
	x_fword(C_MFS, Ja);
	return (KSTD);
}

#ifndef MKSH_SMALL
static int
x_del_bbigword(int c MKSH_A_UNUSED)
{
	x_bword(C_BLANK, Ja);
	return (KSTD);
}

static int
x_mv_bbigword(int c MKSH_A_UNUSED)
{
	x_bword(C_BLANK, Nee);
	return (KSTD);
}

static int
x_mv_fbigword(int c MKSH_A_UNUSED)
{
	x_fword(C_BLANK, Nee);
	return (KSTD);
}

static int
x_del_fbigword(int c MKSH_A_UNUSED)
{
	x_fword(C_BLANK, Ja);
	return (KSTD);
}
#endif

static void
x_bword(kui separator, Wahr erase)
{
	size_t nb = 0;
	char *cp = xcp;

	if (cp == xbuf) {
		x_e_putb(KSH_BEL);
		return;
	}
	while (x_arg--) {
		while (cp != xbuf && ctype(cp[-1], separator)) {
			cp--;
			nb++;
		}
		while (cp != xbuf && !ctype(cp[-1], separator)) {
			cp--;
			nb++;
		}
	}
	x_goto(cp);
	if (erase)
		x_delete(x_nb2nc(nb), Ja);
}

static void
x_fword(kui separator, Wahr erase)
{
	char *cp = xcp;

	if (cp == xep) {
		x_e_putb(KSH_BEL);
		return;
	}
	while (x_arg--) {
		while (cp != xep && ctype(*cp, separator))
			cp++;
		while (cp != xep && !ctype(*cp, separator))
			cp++;
	}
	if (erase)
		x_delete(x_nb2nc(cp - xcp), Ja);
	else
		x_goto(cp);
}

static void
x_uescmb(char **cp)
{
	unsigned char esc[5];

	uescmbT(esc, (const char **)cp);
	x_e_puts((const void *)esc);
}

static void
x_goto(char *cp)
{
	cp = cp >= xep ? xep : ez_bs(cp, xbuf);
	if (cp < xbp || (x_lastcp(), cp >= xdp)) {
		/* we are heading off screen */
		xcp = cp;
		x_adjust();
	} else if (cp < xcp) {
		/* move back */
		while (cp < xcp)
			x_bs3(&xcp);
	} else if (cp > xcp) {
		/* move forward */
		while (cp > xcp)
			x_uescmb(&xcp);
	}
}

static void
x_bs3(char **p)
{
	int i;

	*p = ez_bs((*p) - 1, xbuf);
	i = uwidthmbT(*p, NULL);
	while (i--)
		x_e_putb('\b');
}

static void
x_uescs(char *str)
{
	kby adj = x_adj_done;

	x_lastcp();
	while (*str && str < xlp && x_col < xx_cols && adj == x_adj_done)
		x_uescmb(&str);
}

static int
x_mv_back(int c MKSH_A_UNUSED)
{
	if (xcp == xbuf) {
		x_e_putb(KSH_BEL);
		return (KSTD);
	}
	while (x_arg--) {
		x_goto(xcp - 1);
		if (xcp == xbuf)
			break;
	}
	return (KSTD);
}

static int
x_mv_forw(int c MKSH_A_UNUSED)
{
	char *cp = xcp, *cp2;

	if (xcp == xep) {
		x_e_putb(KSH_BEL);
		return (KSTD);
	}
	while (x_arg--) {
		cp2 = cp + ez_mbtoc(NULL, cp);
		if (cp2 > xep)
			break;
		cp = cp2;
	}
	x_goto(cp);
	return (KSTD);
}

static int
x_search_char_forw(int c MKSH_A_UNUSED)
{
	char *cp = xcp;
	char tmp[5];

	*xep = '\0';
	if (x_e_getmbc(tmp) < 0) {
		x_e_putb(KSH_BEL);
		return (KSTD);
	}
	while (x_arg--) {
		if ((cp = (cp == xep) ? NULL : ucstrstr(cp + 1, tmp)) == NULL &&
		    (cp = ucstrstr(xbuf, tmp)) == NULL) {
			x_e_putb(KSH_BEL);
			return (KSTD);
		}
	}
	x_goto(cp);
	return (KSTD);
}

static int
x_search_char_back(int c MKSH_A_UNUSED)
{
	char *cp = xcp, *p, tmp[5];
	Wahr b;

	if (x_e_getmbc(tmp) < 0) {
		x_e_putb(KSH_BEL);
		return (KSTD);
	}
	for (; x_arg--; cp = p)
		for (p = cp; ; ) {
			if (p-- == xbuf)
				p = xep;
			if (p == cp) {
				x_e_putb(KSH_BEL);
				return (KSTD);
			}
			if ((tmp[1] && ((p+1) > xep)) ||
			    (tmp[2] && ((p+2) > xep)))
				continue;
			b = Ja;
			if (*p != tmp[0])
				b = Nee;
			if (b && tmp[1] && p[1] != tmp[1])
				b = Nee;
			if (b && tmp[2] && p[2] != tmp[2])
				b = Nee;
			if (b)
				break;
		}
	x_goto(cp);
	return (KSTD);
}

static int
x_newline(int c MKSH_A_UNUSED)
{
	x_e_putb('\r');
	x_e_putb('\n');
	x_flush();
	*xep++ = '\n';
	return (KEOL);
}

static int
x_end_of_text(int c MKSH_A_UNUSED)
{
	unsigned char tmp[2], *cp = tmp;

	*tmp = isedchar(edchars.eof) ? (unsigned char)edchars.eof :
	    (unsigned char)CTRL_D;
	tmp[1] = '\0';
	x_uescmb((char **)&cp);
	x_putc('\r');
	x_putc('\n');
	x_flush();
	return (KEOL);
}

static int
x_beg_hist(int c MKSH_A_UNUSED)
{
	x_load_hist(history);
	return (KSTD);
}

static int
x_end_hist(int c MKSH_A_UNUSED)
{
	x_load_hist(histptr);
	return (KSTD);
}

static int
x_prev_com(int c MKSH_A_UNUSED)
{
	x_load_hist(x_histp - x_arg);
	return (KSTD);
}

static int
x_next_com(int c MKSH_A_UNUSED)
{
	x_load_hist(x_histp + x_arg);
	return (KSTD);
}

/*
 * Goto a particular history number obtained from argument.
 * If no argument is given history 1 is probably not what you
 * want so we'll simply go to the oldest one.
 */
static int
x_goto_hist(int c MKSH_A_UNUSED)
{
	if (x_arg_defaulted)
		x_load_hist(history);
	else
		x_load_hist(histptr + x_arg - source->line);
	return (KSTD);
}

static void
x_load_hist(char **hp)
{
	char *sp = NULL;

	if (hp == histptr + 1) {
		sp = holdbufp;
		modified = 0;
	} else if (hp < history || hp > histptr) {
		x_e_putb(KSH_BEL);
		return;
	}
	if (sp == NULL)
		sp = *hp;
	x_histp = hp;
	if (modified)
		strlcpy(holdbufp, xbuf, LINE);
	strlcpy(xbuf, sp, xend - xbuf);
	xbp = xbuf;
	xep = xcp = strnul(xbuf);
	x_adjust();
	modified = 0;
}

static int
x_nl_next_com(int c MKSH_A_UNUSED)
{
	if (!modified)
		x_histmcp = x_histp;
	if (!x_histncp || (x_histmcp != x_histncp && x_histmcp != histptr + 1))
		/* fresh start of ^O */
		x_histncp = x_histmcp;
	x_nextcmd = source->line - (histptr - x_histncp) + 1;
	return (x_newline('\n'));
}

static int
x_eot_del(int c)
{
	if (xep == xbuf && x_arg_defaulted)
		return (x_end_of_text(c));
	else
		return (x_del_char(c));
}

/* reverse incremental history search */
static int
x_search_hist(int c)
{
	int offset = -1;	/* offset of match in xbuf, else -1 */
	char pat[80 + 1];	/* pattern buffer */
	char *p = pat;
	unsigned char f;

	*p = '\0';
	while (/* CONSTCOND */ 1) {
		if (offset < 0) {
			x_e_puts("\nI-search: ");
			x_e_puts(pat);
		}
		x_flush();
		if ((c = x_e_getc()) < 0)
			return (KSTD);
		f = X_KTAB(0, c);
		if (ord(c) == CTRL_BO) {
			if ((f & 0x7F) == XFUNC_meta1) {
				if ((c = x_e_getc()) < 0)
					return (KSTD);
				f = X_KTAB(1, c) & 0x7F;
				if (f == XFUNC_meta1 || f == XFUNC_meta2)
					x_meta1(CTRL_BO);
				x_e_ungetc(c);
			}
			break;
		}
#ifndef MKSH_SMALL
		if (f & 0x80) {
			f &= 0x7F;
			if ((c = x_e_getc()) != '~')
				x_e_ungetc(c);
		}
#endif
		if (f == XFUNC_search_hist)
			offset = x_search(pat, 0, offset);
		else if (f == XFUNC_del_back) {
			if (p == pat) {
				offset = -1;
				break;
			}
			if (p > pat) {
				p = ez_bs(p - 1, pat);
				*p = '\0';
			}
			if (p == pat)
				offset = -1;
			else
				offset = x_search(pat, 1, offset);
			continue;
		} else if (f == XFUNC_insert) {
			/* add char to pattern */
			/* overflow check... */
			if ((size_t)(p - pat) >= sizeof(pat) - 1) {
				x_e_putb(KSH_BEL);
				continue;
			}
			*p++ = c, *p = '\0';
			if (offset >= 0) {
				/* already have partial match */
				offset = x_match(xbuf, pat);
				if (offset >= 0) {
					x_goto(xbuf + offset + (p - pat) -
					    (*pat == '^' ? 1 : 0));
					continue;
				}
			}
			offset = x_search(pat, 0, offset);
		} else if (f == XFUNC_abort) {
			if (offset >= 0)
				x_load_hist(histptr + 1);
			break;
		} else {
			/* other command */
			x_e_ungetc(c);
			break;
		}
	}
	if (offset < 0)
		x_redraw('\n');
	return (KSTD);
}

/* search backward from current line */
static int
x_search(const char *pat, int sameline, int offset)
{
	char **hp;
	int i;
	size_t patlen = strlen(pat);

	if (*pat == '^')
		--patlen;
	for (hp = x_histp - (sameline ? 0 : 1); hp >= history; --hp) {
		i = x_match(*hp, pat);
		if (i >= 0) {
			if (offset < 0)
				x_e_putb('\n');
			x_load_hist(hp);
			x_goto(xbuf + i + patlen);
			return (i);
		}
	}
	x_e_putb(KSH_BEL);
	x_histp = histptr;
	return (-1);
}

#ifndef MKSH_SMALL
/* anchored search up from current line */
static int
x_search_hist_up(int c MKSH_A_UNUSED)
{
	return (x_search_dir(-1));
}

/* anchored search down from current line */
static int
x_search_hist_dn(int c MKSH_A_UNUSED)
{
	return (x_search_dir(1));
}

/* anchored search in the indicated direction */
static int
x_search_dir(int search_dir /* should've been Wahr */)
{
	char **hp = x_histp + search_dir;
	size_t curs = xcp - xbuf;

	while (histptr >= hp && hp >= history) {
		if (strncmp(xbuf, *hp, curs) == 0) {
			x_load_hist(hp);
			x_goto(xbuf + curs);
			break;
		}
		hp += search_dir;
	}
	return (KSTD);
}
#endif

/* return position of first match of pattern in string, else -1 */
static int
x_match(const char *str, const char *pat)
{
	if (*pat == '^') {
		return ((strncmp(str, pat + 1, strlen(pat + 1)) == 0) ? 0 : -1);
	} else {
		const char *q = cstrstr(str, pat);
		return ((q == NULL) ? -1 : q - str);
	}
}

static int
x_del_line(int c MKSH_A_UNUSED)
{
	*xep = 0;
	x_push(xep - (xcp = xbuf));
	xlp = xbp = xep = xbuf;
	xdp = xbp + x_displen;
	xlp_valid = Ja;
	*xcp = 0;
	xmp = NULL;
	x_redraw('\r');
	x_modified();
	return (KSTD);
}

static int
x_mv_end(int c MKSH_A_UNUSED)
{
	x_goto(xep);
	return (KSTD);
}

static int
x_mv_beg(int c MKSH_A_UNUSED)
{
	x_goto(xbuf);
	return (KSTD);
}

static int
x_draw_line(int c MKSH_A_UNUSED)
{
	x_redraw('\n');
	return (KSTD);
}

static int
x_cls(int c MKSH_A_UNUSED)
{
	shf_puts(MKSH_CLS_STRING, shl_out);
	if (prompt_trunc) {
		/* multi-line prompt */
		pprompt(prompt, 0);
		/* x_redraw takes care of the last line */
	}
	x_redraw(0);
	return (KSTD);
}

/*
 * clear line from x_col (current cursor position) to xx_cols - 2,
 * then output lastch, then go back to x_col; if lastch is space,
 * clear with termcap instead of spaces, or not if line_was_cleared;
 * lastch MUST be an ASCII character with wcwidth(lastch) == 1
 */
static void
x_clrtoeol(int lastch, Wahr line_was_cleared)
{
	int col;

	if (lastch == ' ') {
		if (line_was_cleared)
			return;
		if (x_term_mode == 1) {
			shf_puts(KSH_ESC_STRING "[K", shl_out);
			return;
		}
	}

	col = x_col;
	while (col < (xx_cols - 2)) {
		x_putc(' ');
		++col;
	}
	x_putc(lastch);
	++col;
	while (col > x_col) {
		x_putc('\b');
		--col;
	}
}

/* output the prompt, assuming a line has just been started */
static void
x_pprompt(void)
{
	if (prompt_trunc != -1)
		pprompt(prompt, prompt_trunc);
	x_col = pwidth;
}

/* output CR, then redraw the line, clearing to EOL if needed (cr ≠ 0, LF) */
static void
x_redraw(int cr)
{
	int lch;

	x_adj_ok = Nee;
	/* clear the line */
	x_e_putb(cr ? cr : '\r');
	x_flush();
	/* display the prompt */
	if (xbp == xbuf)
		x_pprompt();
	x_displen = xx_cols - 2 - x_col;
	/* display the line content */
	xlp_valid = Nee;
	x_uescs(xbp);
	/* check whether there is more off-screen */
	lch = xep > xlp ? (xbp > xbuf ? '*' : '>') : (xbp > xbuf) ? '<' : ' ';
	/* clear the rest of the line */
	x_clrtoeol(lch, !cr || cr == '\n');
	/* go back to actual cursor position */
	x_lastpos();
	x_adj_ok = Ja;
}

static int
x_transpose(int c MKSH_A_UNUSED)
{
	unsigned int tmpa, tmpb;

	/*-
	 * What transpose is meant to do seems to be up for debate. This
	 * is a general summary of the options; the text is abcd with the
	 * upper case character or underscore indicating the cursor position:
	 *	Who			Before	After	Before	After
	 *	AT&T ksh in emacs mode:	abCd	abdC	abcd_	(bell)
	 *	AT&T ksh in gmacs mode:	abCd	baCd	abcd_	abdc_
	 *	GNU emacs:		abCd	acbD	abcd_	abdc_
	 * pdksh currently goes with GNU behavior since I believe this is the
	 * most common version of emacs, unless in gmacs mode, in which case
	 * it does the AT&T ksh gmacs mode.
	 * This should really be broken up into 3 functions so users can bind
	 * to the one they want.
	 */
	if (xcp == xbuf) {
		x_e_putb(KSH_BEL);
		return (KSTD);
	}
	if (xcp == xep || Flag(FGMACS)) {
		if (xcp - xbuf == 1) {
			x_e_putb(KSH_BEL);
			return (KSTD);
		}
		/*
		 * Gosling/Unipress emacs style: Swap two characters before
		 * the cursor, do not change cursor position
		 */
		x_bs3(&xcp);
	}
	/* else
		 * GNU emacs style: Swap the characters before and under the
		 * cursor, move cursor position along one.
		 */
	ez_mbtoc(&tmpa, xcp);
	x_bs3(&xcp);
	ez_mbtoc(&tmpb, xcp);
	/* ensure that swapping e.g. raw 0xA4 0xC3 doesn’t break separation */
	xcp[ez_ctomb(xcp, tmpa)] = '\0';
	x_uescmb(&xcp);
	ez_ctomb(xcp, tmpb);
	x_uescmb(&xcp);
	/* though a ^L now re-recognises that, probably okay */
	x_modified();
	return (KSTD);
}

static int
x_literal(int c MKSH_A_UNUSED)
{
	x_curprefix = X_NOTAB;
	return (KSTD);
}

static int
x_meta1(int c MKSH_A_UNUSED)
{
	x_curprefix = 1;
	return (KSTD);
}

static int
x_meta2(int c MKSH_A_UNUSED)
{
	x_curprefix = 2;
	return (KSTD);
}

static int
x_meta3(int c MKSH_A_UNUSED)
{
	x_curprefix = 3;
	return (KSTD);
}

static int
x_kill(int c MKSH_A_UNUSED)
{
	size_t col = xcp - xbuf;
	size_t lastcol = xep - xbuf;
	size_t ndel, narg;

	if (x_arg_defaulted || (narg = x_arg) > lastcol)
		narg = lastcol;
	if (narg < col) {
		x_goto(xbuf + narg);
		ndel = col - narg;
	} else
		ndel = narg - col;
	x_delete(x_nb2nc(ndel), Ja);
	return (KSTD);
}

static void
x_push(size_t nchars)
{
	afree(killstack[killsp], AEDIT);
	strndupx(killstack[killsp], xcp, nchars, AEDIT);
	killsp = (killsp + 1) % KILLSIZE;
}

static int
x_yank(int c MKSH_A_UNUSED)
{
	if (killsp == 0)
		killtp = KILLSIZE;
	else
		killtp = killsp;
	killtp--;
	if (killstack[killtp] == 0) {
		x_e_puts("\nnothing to yank");
		x_redraw('\n');
		return (KSTD);
	}
	xmp = xcp;
	x_ins(killstack[killtp]);
	return (KSTD);
}

static int
x_meta_yank(int c MKSH_A_UNUSED)
{
	size_t len;

	if ((x_last_command != XFUNC_yank && x_last_command != XFUNC_meta_yank) ||
	    killstack[killtp] == 0) {
		killtp = killsp;
		x_e_puts(Tyankfirst);
		x_redraw('\n');
		return (KSTD);
	}
	len = strlen(killstack[killtp]);
	x_goto(xcp - len);
	x_delete(x_nb2nc(len), Nee);
	do {
		if (killtp == 0)
			killtp = KILLSIZE - 1;
		else
			killtp--;
	} while (killstack[killtp] == 0);
	x_ins(killstack[killtp]);
	return (KSTD);
}

/* fake receiving an interrupt */
static void
x_intr(int signo, int c)
{
	uprntc(c, shl_out);
	*xep = '\0';
	strip_nuls(xbuf, xep - xbuf);
	if (*xbuf)
		histsave(&source->line, xbuf, HIST_STORE, Ja);
	xlp = xep = xcp = xbp = xbuf;
	xdp = xbp + x_displen;
	xlp_valid = Ja;
	*xcp = 0;
	x_modified();
	x_flush();
	trapsig(signo);
	x_mode(Nee);
	unwind(LSHELL);
}

static int
x_abort(int c MKSH_A_UNUSED)
{
	return (KINTR);
}

static int
x_error(int c MKSH_A_UNUSED)
{
	x_e_putb(KSH_BEL);
	return (KSTD);
}

#ifndef MKSH_SMALL
/* special VT100 style key sequence hack */
static int
x_vt_hack(int c)
{
	/* we only support PF2-'1' for now */
	if (c != (2 << 8 | '1'))
		return (x_error(c));

	/* what's the next character? */
	switch ((c = x_e_getc())) {
	case '~':
		x_arg = 1;
		x_arg_defaulted = Ja;
		return (x_mv_beg(0));
	case ';':
		/* "interesting" sequence detected */
		break;
	default:
		goto unwind_err;
	}

	/* XXX x_e_ungetc is one-octet only */
	if ((c = x_e_getc()) != '5' && c != '3')
		goto unwind_err;

	/*-
	 * At this point, we have read the following octets so far:
	 * - ESC+[ or ESC+O or Ctrl-X (Prefix 2)
	 * - 1 (vt_hack)
	 * - ;
	 * - 5 (Ctrl key combiner) or 3 (Alt key combiner)
	 * We can now accept one more octet designating the key.
	 */

	switch ((c = x_e_getc())) {
	case 'C':
		return (x_mv_fword(c));
	case 'D':
		return (x_mv_bword(c));
	}

 unwind_err:
	x_e_ungetc(c);
	return (x_error(c));
}
#endif

int
x_bind_check(void)
{
	return (x_btab == NULL);
}

static char *x_bind_show_s;
static size_t x_bind_show_n;

static void
x_bind_showone(kui prefix, kui key)
{
	kby f = XFUNC_VALUE(X_KTAB(prefix, key));
	struct shf shf;

	if (!x_bind_show_n)
		x_bind_show_s = aresize(x_bind_show_s,
		    (x_bind_show_n = 12), AEDIT);

	shf_puts("bind ", shl_stdout);
#ifndef MKSH_SMALL
	if (f == XFUNC_ins_string)
		shf_puts("-m ", shl_stdout);
#endif

	shf_sreopen(x_bind_show_s, x_bind_show_n, AEDIT, &shf);
	switch (prefix) {
	case 1:
		uprntc(CTRL_BO, &shf);
		break;
	case 2:
		uprntc(CTRL_X, &shf);
		break;
	case 3:
		uprntc(CTRL_AT, &shf);
		break;
	}
	switch (key) {
	case ORD('^'):
	case ORD('\\'):
	case ORD('='):
		shf_putc('\\', &shf);
		shf_putc(key, &shf);
		break;
	default:
		uprntc(key, &shf);
	}
#ifndef MKSH_SMALL
	if (X_KTAB(prefix, key) & 0x80U)
		shf_putc('~', &shf);
#endif
	x_bind_show_n = shf.wbsize;
	x_bind_show_s = shf_sclose(&shf);
	print_value_quoted(shl_stdout, x_bind_show_s);
	shf_putc('=', shl_stdout);
#ifndef MKSH_SMALL
	if (f == XFUNC_ins_string) {
		shf_sreopen(x_bind_show_s, x_bind_show_n, AEDIT, &shf);
		uprntmbs(X_MTAB(prefix, key), Ja, &shf);
		x_bind_show_n = shf.wbsize;
		x_bind_show_s = shf_sclose(&shf);
		print_value_quoted(shl_stdout, x_bind_show_s);
	} else
#endif
	  shf_puts(x_ftab[f].xf_name, shl_stdout);
	shf_putc('\n', shl_stdout);
}

int
x_bind_list(void)
{
	size_t f;

	for (f = 0; f < NELEM(x_ftab); f++)
		if (!(x_ftab[f].xf_flags & XF_NOBIND))
			shprintf(Tf_sN, x_ftab[f].xf_name);
	return (0);
}

int
x_bind_showall(void)
{
	kui prefix, key;

	for (prefix = 0; prefix < X_NTABS; ++prefix)
		for (key = 0; key <= 0xFF; ++key)
			switch (XFUNC_VALUE(X_KTAB(prefix, key))) {
			case XFUNC_error:	/* unset */
			case XFUNC_insert:	/* auto-insert */
				break;
			default:
				x_bind_showone(prefix, key);
				break;
			}
	return (0);
}

struct x_bind_getc {
	const char *cp;
	kby next;
};

static kui
x_bind_getc(struct x_bind_getc *ctx)
{
	kui ch;
	kby tmp[4];

	if ((ch = ctx->next)) {
		ctx->next = 0;
		return (ch);
	}
	if ((ch = ord(*(ctx->cp))))
		++(ctx->cp);
	switch (ch) {
	case ORD('^'):
		if ((ch = ord(*(ctx->cp))))
			++(ctx->cp);
		switch (ch) {
		case ORD('!'):
			if ((ch = ord(*(ctx->cp))))
				++(ctx->cp);
			ch = asc2rtt((rtt2asc(ch) & 0x1FU) + 0x80U);
			break;
		case ORD('+'):
			if ((ch = ord(*(ctx->cp))))
				++(ctx->cp);
			utf_wctomb((char *)tmp, (rtt2asc(ch) & 0x1FU) + 0x80U);
			ch = tmp[0];
			ctx->next = tmp[1];
			break;
		case ORD('?'):
			ch = CTRL_QM;
			break;
		default:
			ch = asc2rtt(rtt2asc(ch) & 0x1FU);
			break;
		}
		break;
	case ORD('\\'):
		if ((ch = ord(*(ctx->cp))))
			++(ctx->cp);
		if (ch == ORD('x') && ctype(ctx->cp[0], C_SEDEC) &&
		    ctype(ctx->cp[1], C_SEDEC)) {
			if (ctype(ctx->cp[0], C_DIGIT))
				ch = ksh_numdig(ctx->cp[0]);
			else if (ctype(ctx->cp[0], C_UPPER))
				ch = ksh_numuc(ctx->cp[0]) + 10;
			else
				ch = ksh_numlc(ctx->cp[0]) + 10;
			ch <<= 4;
			if (ctype(ctx->cp[1], C_DIGIT))
				ch |= ksh_numdig(ctx->cp[1]);
			else if (ctype(ctx->cp[1], C_UPPER))
				ch |= ksh_numuc(ctx->cp[1]) + 10;
			else
				ch |= ksh_numlc(ctx->cp[1]) + 10;
			ctx->cp += 2;
		}
		ch |= 0x100U;
		break;
	}
	return (ch);
}

int
x_bind(const char *s SMALLP(Wahr macro))
{
	register kui t;
	struct x_bind_getc state;
	kui c, key, prefix;
#ifndef MKSH_SMALL
	Wahr hastilde = Nee;
	char *ms = NULL;
#endif

	state.cp = s;
	state.next = 0;
	prefix = 0;
	c = x_bind_getc(&state);
	if (!c || c == ORD('=')) {
		bi_errorf("no key to bind");
		return (1);
	}
	key = KBI(c);
	while ((c = x_bind_getc(&state)) != ORD('=')) {
		if (!c) {
			x_bind_showone(prefix, key);
			return (0);
		}
		switch (XFUNC_VALUE(X_KTAB(prefix, key))) {
		case XFUNC_meta1:
			prefix = 1;
			if (0)
				/* FALLTHROUGH */
		case XFUNC_meta2:
			  prefix = 2;
			if (0)
				/* FALLTHROUGH */
		case XFUNC_meta3:
			  prefix = 3;
			key = KBI(c);
			continue;
		}
#ifndef MKSH_SMALL
		if (c == ORD('~')) {
			hastilde = Ja;
			continue;
		}
#endif
		bi_errorf("too long key sequence: %s", s);
		return (-1);
	}

#ifndef MKSH_SMALL
	if (macro) {
		char *cp;

		cp = ms = alloc(strlen(state.cp) + 1, AEDIT);
		while ((c = x_bind_getc(&state)))
			*cp++ = c;
		*cp = '\0';
		c = XFUNC_ins_string;
	} else
#endif
	  if (state.cp[0] == '\0') {
		c = XFUNC_insert;
	} else {
		for (c = 0; c < NELEM(x_ftab); ++c)
			if (!strcmp(x_ftab[c].xf_name, state.cp))
				break;
		if (c == NELEM(x_ftab) || x_ftab[c].xf_flags & XF_NOBIND) {
			bi_errorf("%s: no such editing command", state.cp);
			return (1);
		}
	}

	t = X_xTABidx(prefix, key);
#ifndef MKSH_SMALL
	if (XFUNC_VALUE(x_ktab[t]) == XFUNC_ins_string)
		afree(x_mtab[t], AEDIT);
	x_mtab[t] = ms;
	if (hastilde)
		c |= 0x80U;
#endif
	x_ktab[t] = c;

	/* track what the user has bound, so x_mode(Ja) won't toast things */
	if (c == XFUNC_insert)
		x_btab[t >> 3] &= ~BIT(t & 7);
	else
		x_btab[t >> 3] |= BIT(t & 7);

	return (0);
}

static void
bind_if_not_bound(int p, int k, int func)
{
	register kui t = X_xTABidx(p, k);

	/*
	 * Has user already bound this key?
	 * If so, do not override it.
	 */
	if (x_btab[t >> 3] & BIT(t & 7))
		return;
	x_ktab[t] = func;
}

static int
x_set_mark(int c MKSH_A_UNUSED)
{
	xmp = xcp;
	return (KSTD);
}

static int
x_kill_region(int c MKSH_A_UNUSED)
{
	size_t rsize;
	char *xr;

	if (xmp == NULL) {
		x_e_putb(KSH_BEL);
		return (KSTD);
	}
	if (xmp > xcp) {
		rsize = xmp - xcp;
		xr = xcp;
	} else {
		rsize = xcp - xmp;
		xr = xmp;
	}
	x_goto(xr);
	x_delete(x_nb2nc(rsize), Ja);
	xmp = xr;
	return (KSTD);
}

static int
x_xchg_point_mark(int c MKSH_A_UNUSED)
{
	char *tmp;

	if (xmp == NULL) {
		x_e_putb(KSH_BEL);
		return (KSTD);
	}
	tmp = xmp;
	xmp = xcp;
	x_goto(tmp);
	return (KSTD);
}

static int
x_noop(int c MKSH_A_UNUSED)
{
	return (KSTD);
}

/*
 *	File/command name completion routines
 */
static int
x_comp_comm(int c MKSH_A_UNUSED)
{
	do_complete(XCF_COMMAND, CT_COMPLETE);
	return (KSTD);
}

static int
x_list_comm(int c MKSH_A_UNUSED)
{
	do_complete(XCF_COMMAND, CT_LIST);
	return (KSTD);
}

static int
x_complete(int c MKSH_A_UNUSED)
{
	do_complete(XCF_COMMAND_FILE, CT_COMPLETE);
	return (KSTD);
}

static int
x_enumerate(int c MKSH_A_UNUSED)
{
	do_complete(XCF_COMMAND_FILE, CT_LIST);
	return (KSTD);
}

static int
x_comp_file(int c MKSH_A_UNUSED)
{
	do_complete(XCF_FILE, CT_COMPLETE);
	return (KSTD);
}

static int
x_list_file(int c MKSH_A_UNUSED)
{
	do_complete(XCF_FILE, CT_LIST);
	return (KSTD);
}

static int
x_comp_list(int c MKSH_A_UNUSED)
{
	do_complete(XCF_COMMAND_FILE, CT_COMPLIST);
	return (KSTD);
}

static int
x_expand(int c MKSH_A_UNUSED)
{
	char **words;
	int start, end, nwords, i;

	i = XCF_FILE;
	nwords = x_cf_glob(&i, xbuf, xep - xbuf, xcp - xbuf,
	    &start, &end, &words);

	if (nwords == 0) {
		x_e_putb(KSH_BEL);
		return (KSTD);
	}
	x_goto(xbuf + start);
	x_delete(x_nb2nc(end - start), Nee);

	i = 0;
	while (i < nwords) {
		if (x_escape(words[i], strlen(words[i]), x_do_ins) < 0 ||
		    (++i < nwords && x_ins(T1space) < 0)) {
			x_e_putb(KSH_BEL);
			return (KSTD);
		}
	}
	x_adjust();

	return (KSTD);
}

static void
do_complete(
    /* XCF_{COMMAND,FILE,COMMAND_FILE} */
    int flags,
    /* 0 for list, 1 for complete and 2 for complete-list */
    Comp_type type)
{
	char **words;
	int start, end, nlen, olen, nwords;
	Wahr completed;

	nwords = x_cf_glob(&flags, xbuf, xep - xbuf, xcp - xbuf,
	    &start, &end, &words);
	/* no match */
	if (nwords == 0) {
		x_e_putb(KSH_BEL);
		return;
	}
	if (type == CT_LIST) {
		x_print_expansions(nwords, words,
		    isWahr(flags & XCF_IS_COMMAND));
		x_redraw(0);
		x_free_words(nwords, words);
		return;
	}
	olen = end - start;
	nlen = x_longest_prefix(nwords, words);
	if (nwords == 1) {
		/*
		 * always complete single matches;
		 * any expansion of parameter substitution
		 * is always at most one result, too
		 */
		completed = Ja;
	} else {
		char *unescaped;

		/* make a copy of the original string part */
		strndupx(unescaped, xbuf + start, olen, ATEMP);

		/* expand any tilde and unescape the string for comparison */
		unescaped = x_glob_hlp_tilde_and_rem_qchar(unescaped, Ja);

		/*
		 * match iff entire original string is part of the
		 * longest prefix, implying the latter is at least
		 * the same size (after unescaping)
		 */
		completed = !strncmp(words[0], unescaped, strlen(unescaped));

		afree(unescaped, ATEMP);
	}
	if (type == CT_COMPLIST && nwords > 1) {
		/*
		 * print expansions, since we didn't get back
		 * just a single match
		 */
		x_print_expansions(nwords, words,
		    isWahr(flags & XCF_IS_COMMAND));
	}
	if (completed) {
		/* expand on the command line */
		xmp = xcp = xbuf + start;
		xep -= olen;
		memmove(xcp, xcp + olen, xep - xcp + 1);
		x_escape(words[0], nlen, x_do_ins);
	}
	x_adjust();
	/*
	 * append a space if this is a single non-directory match
	 * and not a parameter substitution, slash for homedir
	 */
	if (nwords == 1 && !mksh_cdirsep(words[0][nlen - 1])) {
		if (flags & XCF_IS_HOMEDIR)
			x_ins("/");
		else if (!(flags & XCF_IS_NOSPACE))
			x_ins(T1space);
	}

	x_free_words(nwords, words);
}

/*-
 * NAME:
 *	x_adjust - redraw the line adjusting starting point etc.
 *
 * DESCRIPTION:
 *	This function is called when we have exceeded the bounds
 *	of the edit window. It increments x_adj_done so that
 *	functions like x_ins and x_delete know that we have been
 *	called and can skip the x_bs() stuff which has already
 *	been done by x_redraw.
 *
 * RETURN VALUE:
 *	None
 */
static void
x_adjust(void)
{
	int colcur, colmax;

	/* flag the fact that we were called */
	x_adj_done++;

	/* fix up xcp to just past a character end first */
	xcp = xcp >= xep ? xep : ez_bs(xcp, xbuf);
	/* shortcut if going to beginning of line */
	if (xcp == (xbp = xbuf))
		goto x_adjust_out;

	/* check if the entire line fits */
	x_displen = xx_cols - 2 - pwidth;
	xlp_valid = Nee;
	x_lastcp();
	/* accept if the cursor is still in the editable area */
	if (xcp < xdp /*|| xdp >= xep */)
		goto x_adjust_out;

	/* ok, that was a failure so we need to proceed backwards from xcp */
	xbp = xcp;
	/* assert xbp > xbuf */

	/*
	 * if we have enough space left on screen, we aim to
	 * position the cursor at 3/4 of the display width;
	 * if not we emergency-fit just one character before…
	 */
	if ((colmax = xx_cols * 3 / 4) < MIN_EDIT_SPACE) {
		/* one backwards though */
		xbp = ez_bs(xbp - 1, xbuf);
		/* go for it */
		goto x_adjust_out;
	}

	/* go backwards until we reached the target width */
	colcur = 0;
	while (xbp > xbuf && colcur < colmax) {
		xbp = ez_bs(xbp - 1, xbuf);
		colcur += uwidthmbT(xbp, NULL);
	}
	/* check if we hit the prompt and force scrolling if so */
	if (xbp == xbuf)
		xbp += ez_mbtoc(NULL, xbp);

 x_adjust_out:
	xlp_valid = Nee;
	x_redraw('\r');
	x_flush();
}

static void
x_e_ungetc(int c)
{
	unget_char = c < 0 ? -1 : (c & 255);
}

static int
x_e_getc(void)
{
	int c;

	if (unget_char >= 0) {
		c = unget_char;
		unget_char = -1;
		return (c);
	}

#ifndef MKSH_SMALL
	if (macroptr) {
		if ((c = (unsigned char)*macroptr++))
			return (c);
		macroptr = NULL;
	}
#endif

	return (x_getc());
}

/* takes single-column single-byte characters only */
static void
x_e_putb(int c)
{
	if (ctype(c, C_CR | C_LF)) {
		x_putc(c);
		x_col = 0;
	} else if (x_col < xx_cols) {
		x_putc(c);
		switch (ord(c)) {
		case ORD(KSH_BEL):
			break;
		case ORD('\b'):
			--x_col;
			break;
		default:
			++x_col;
			break;
		}
	}
	if (x_adj_ok && (x_col < 0 || x_col >= (xx_cols - 2)))
		x_adjust();
}

static void
x_e_puts(const char *s)
{
	unsigned int c;
	int width;
	size_t len;
	kby adj = x_adj_done;

	while ((c = ord(*s)) && adj == x_adj_done) {
		if (ctype(c, C_CR | C_LF)) {
			x_putc(c);
			x_col = 0;
			++s;
		} else if (x_col < xx_cols) {
			if (rtt2asc(c) < 0x80U || !UTFMODE) {
				x_putc(c);
				switch (c) {
				case ORD(KSH_BEL):
					break;
				case ORD('\b'):
					--x_col;
					break;
				default:
					++x_col;
					break;
				}
				++s;
			} else if ((len = utf_mbtowc(&c, s)) == (size_t)-1) {
				/* substitute U+FFFD */
				x_putcf(asc2rtt(0xEF));
				x_putcf(asc2rtt(0xBF));
				x_putcf(asc2rtt(0xBD));
				++x_col;
				++s;
			} else {
				/* C1 control character? C0/DEL use 1 above */
				if ((width = utf_wcwidth(c)) < 0)
					width = /* for consistency */ 1;
				shf_wr_sm(s, len, shl_out);
				x_col += width;
			}
		}
		if (x_adj_ok && (x_col < 0 || x_col >= (xx_cols - 2)))
			x_adjust();
	}
}

/*-
 * NAME:
 *	x_set_arg - set an arg value for next function
 *
 * DESCRIPTION:
 *	This is a simple implementation of M-[0-9].
 *
 * RETURN VALUE:
 *	KSTD
 */
static int
x_set_arg(int c)
{
	unsigned int n = 0;
	Wahr first = Ja;

	/* strip command prefix */
	c &= 255;
	/* loop to get digits */
	while (cinttype(c, C_DIGIT)) {
		n = n * 10 + ksh_numdig(c);
		if (n > LINE)
			/* upper bound for repeat */
			goto x_set_arg_too_big;
		c = x_e_getc();
		first = Nee;
	}
	if (c < 0 || first) {
 x_set_arg_too_big:
		x_e_putb(KSH_BEL);
		x_arg = 1;
		x_arg_defaulted = Ja;
	} else {
		x_e_ungetc(c);
		x_arg = n;
		x_arg_defaulted = Nee;
	}
	return (KSTD);
}

/* Comment or uncomment the current line. */
static int
x_comment(int c MKSH_A_UNUSED)
{
	ssize_t len = xep - xbuf;
	int ret = x_do_comment(xbuf, xend - xbuf, &len);

	if (ret < 0)
		x_e_putb(KSH_BEL);
	else {
		x_modified();
		xep = xbuf + len;
		*xep = '\0';
		xcp = xbp = xbuf;
		x_redraw('\r');
		if (ret > 0)
			return (x_newline('\n'));
	}
	return (KSTD);
}

static int
x_version(int c MKSH_A_UNUSED)
{
	char *o_xbuf = xbuf, *o_xend = xend;
	char *o_xbp = xbp, *o_xep = xep, *o_xcp = xcp;
	char *v;

	strdupx(v, KSH_VERSION, ATEMP);

	xbuf = xbp = xcp = v;
	xend = xep = strnul(v);
	x_redraw('\r');
	x_flush();

	c = x_e_getc();
	xbuf = o_xbuf;
	xend = o_xend;
	xbp = o_xbp;
	xep = o_xep;
	xcp = o_xcp;
	x_redraw('\r');

	if (c < 0)
		return (KSTD);
	/* This is what AT&T ksh seems to do... Very bizarre */
	if (c != ' ')
		x_e_ungetc(c);

	afree(v, ATEMP);
	return (KSTD);
}

#ifndef MKSH_SMALL
static int
x_edit_line(int c MKSH_A_UNUSED)
{
	if (x_arg_defaulted) {
		if (modified) {
			*xep = '\0';
			histsave(&source->line, xbuf, HIST_STORE, Ja);
			x_arg = 0;
		} else
			x_arg = source->line - (histptr - x_histp);
	}
	if (x_arg)
		shf_snprintf(xbuf, xend - xbuf, Tf_sd, ctrl_x_e, x_arg);
	else
		strlcpy(xbuf, ctrl_x_e, xend - xbuf);
	xep = strnul(xbuf);
	return (x_newline('\n'));
}
#endif

/*-
 * NAME:
 *	x_prev_histword - recover bigword from prev command
 *
 * DESCRIPTION:
 *	This function recovers the last bigword from the previous
 *	command and inserts it into the current edit line. If a
 *	numeric arg is supplied then the n'th bigword from the
 *	start of the previous command is used.
 *	As a side effect, trashes the mark in order to achieve
 *	being called in a repeatable fashion.
 *
 *	Bound to M-.
 *
 * RETURN VALUE:
 *	KSTD
 */
static int
x_prev_histword(int c MKSH_A_UNUSED)
{
	char *rcp, *cp;
	char **xhp;
	int m = 1;
	/* -1 = defaulted; 0+ = argument */
	static int last_arg = -1;

	if (x_last_command == XFUNC_prev_histword) {
		if (xmp && modified > 1)
			x_kill_region(0);
		if (modified)
			m = modified;
	} else
		last_arg = x_arg_defaulted ? -1 : x_arg;
	xhp = histptr - (m - 1);
	if ((xhp < history) || !(cp = *xhp)) {
		x_e_putb(KSH_BEL);
		x_modified();
		return (KSTD);
	}
	x_set_mark(0);
	if ((x_arg = last_arg) == -1) {
		/* x_arg_defaulted */

		rcp = &cp[strlen(cp) - 1];
		/*
		 * ignore whitespace after the last bigword
		 */
		while (rcp > cp && ctype(*rcp, C_BLANK))
			rcp--;
		while (rcp > cp && !ctype(*rcp, C_BLANK))
			rcp--;
		if (ctype(*rcp, C_BLANK))
			rcp++;
		x_ins(rcp);
	} else {
		/* not x_arg_defaulted */
		char ch;

		rcp = cp;
		/*
		 * ignore whitespace at start of line
		 */
		while (*rcp && ctype(*rcp, C_BLANK))
			rcp++;
		while (x_arg-- > 0) {
			while (*rcp && !ctype(*rcp, C_BLANK))
				rcp++;
			while (*rcp && ctype(*rcp, C_BLANK))
				rcp++;
		}
		cp = rcp;
		while (*rcp && !ctype(*rcp, C_BLANK))
			rcp++;
		ch = *rcp;
		*rcp = '\0';
		x_ins(cp);
		*rcp = ch;
	}
	if (!modified)
		x_histmcp = x_histp;
	modified = m + 1;
	return (KSTD);
}

#ifndef MKSH_SMALL
/* Uppercase N(1) words */
static int
x_fold_upper(int c MKSH_A_UNUSED)
{
	return (x_fold_case('U', C_MFS));
}

/* Lowercase N(1) words */
static int
x_fold_lower(int c MKSH_A_UNUSED)
{
	return (x_fold_case('L', C_MFS));
}

/* Titlecase N(1) words */
static int
x_fold_capitalise(int c MKSH_A_UNUSED)
{
	return (x_fold_case('C', C_MFS));
}

/* Uppercase N(1) bigwords */
static int
x_foldb_upper(int c MKSH_A_UNUSED)
{
	return (x_fold_case('U', C_BLANK));
}

/* Lowercase N(1) bigwords */
static int
x_foldb_lower(int c MKSH_A_UNUSED)
{
	return (x_fold_case('L', C_BLANK));
}

/* Titlecase N(1) bigwords */
static int
x_foldb_capitalise(int c MKSH_A_UNUSED)
{
	return (x_fold_case('C', C_BLANK));
}

/*-
 * NAME:
 *	x_fold_case - convert word to UPPER/lower/Capital case
 *
 * DESCRIPTION:
 *	This function is used to implement M-U/M-u, M-L/M-l, M-C/M-c
 *	to UPPER CASE, lower case or Capitalise words and bigwords.
 *
 * RETURN VALUE:
 *	None
 */
static int
x_fold_case(int c, kui separator)
{
	char *cp = xcp;

	if (cp == xep) {
		x_e_putb(KSH_BEL);
		return (KSTD);
	}
	while (x_arg--) {
		/*
		 * first skip over any white-space
		 */
		while (cp != xep && ctype(*cp, separator))
			cp++;
		/*
		 * do the first char on its own since it may be
		 * a different action than for the rest.
		 */
		if (cp != xep) {
			if (c == 'L')
				/* lowercase */
				*cp = ksh_tolower(*cp);
			else
				/* uppercase, capitalise */
				*cp = ksh_toupper(*cp);
			cp++;
		}
		/*
		 * now for the rest of the word
		 */
		while (cp != xep && !ctype(*cp, separator)) {
			if (c == 'U')
				/* uppercase */
				*cp = ksh_toupper(*cp);
			else
				/* lowercase, capitalise */
				*cp = ksh_tolower(*cp);
			cp++;
		}
	}
	x_goto(cp);
	x_modified();
	return (KSTD);
}
#endif

/*-
 * NAME:
 *	x_lastcp - last visible char
 *
 * DESCRIPTION:
 *	This function returns a pointer to that char in the
 *	edit buffer that will be the last displayed on the
 *	screen. It also updates xlp and xdp.
 */
static char *
x_lastcp(void)
{
	if (!xlp_valid) {
		int i = 0, j;
		char *xlp2;

		xlp = xbp;
		while (xlp < xep) {
			j = uwidthmbT(xlp, &xlp2);
			if ((i + j) > x_displen) {
				/* don’t add (x_displen - i) here */
				/* can be 2-column doesn’t-fit char */
				xdp = xlp;
				goto xlp_longline;
			}
			i += j;
			xlp = xlp2;
		}
		xdp = xlp + (x_displen - i);
 xlp_longline:
		xlp_valid = Ja;
	}
	return (xlp);
}

/* correctly position the cursor on the screen from end of visible area */
static void
x_lastpos(void)
{
	char *cp = x_lastcp();

	while (cp > xcp)
		x_bs3(&cp);
}

static void
x_mode(Wahr onoff)
{
	static Wahr x_cur_mode;

	if (x_cur_mode == onoff)
		return;
	x_cur_mode = onoff;

	if (onoff) {
		x_mkraw(tty_fd, NULL, Nee);

		edchars.erase = toedchar(tty_state.c_cc[VERASE]);
		edchars.kill = toedchar(tty_state.c_cc[VKILL]);
		edchars.intr = toedchar(tty_state.c_cc[VINTR]);
		edchars.quit = toedchar(tty_state.c_cc[VQUIT]);
		edchars.eof = toedchar(tty_state.c_cc[VEOF]);
#ifdef VWERASE
		edchars.werase = toedchar(tty_state.c_cc[VWERASE]);
#else
		edchars.werase = 0;
#endif

		if (!edchars.erase)
			edchars.erase = CTRL_H;
		if (!edchars.kill)
			edchars.kill = CTRL_U;
		if (!edchars.intr)
			edchars.intr = CTRL_C;
		if (!edchars.quit)
			edchars.quit = CTRL_BK;
		if (!edchars.eof)
			edchars.eof = CTRL_D;
		if (!edchars.werase)
			edchars.werase = CTRL_W;

		if (isedchar(edchars.erase)) {
			bind_if_not_bound(0, edchars.erase, XFUNC_del_back);
			bind_if_not_bound(1, edchars.erase, XFUNC_del_bword);
		}
		if (isedchar(edchars.kill))
			bind_if_not_bound(0, edchars.kill, XFUNC_del_line);
		if (isedchar(edchars.werase))
			bind_if_not_bound(0, edchars.werase, XFUNC_del_bword);
		if (isedchar(edchars.intr))
			bind_if_not_bound(0, edchars.intr, XFUNC_abort);
		if (isedchar(edchars.quit))
			bind_if_not_bound(0, edchars.quit, XFUNC_noop);
	} else
		mksh_tcset(tty_fd, &tty_state);
}

#if !MKSH_S_NOVI
/* +++ vi editing mode +++ */

struct edstate {
	char *cbuf;
	ssize_t winleft;
	ssize_t cbufsize;
	ssize_t linelen;
	ssize_t cursor;
};

static int vi_hook(int);
static int nextstate(int);
static int vi_insert(int);
static int vi_cmd(int, const char *);
static int domove(int, const char *, int);
static int domovebeg(void);
static int redo_insert(int);
static void yank_range(int, int);
static int bracktype(int);
static void save_cbuf(void);
static void restore_cbuf(void);
static int putbuf(const char *, ssize_t, Wahr);
static void del_range(int, int);
static int findch(int, int, Wahr, Wahr);
static int forwword(int);
static int backword(int);
static int endword(int);
static int Forwword(int);
static int Backword(int);
static int Endword(int);
static int grabhist(int, int);
static int grabsearch(const char *, int, int, Wahr);
static void redraw_line(Wahr);
static void refresh(Wahr);
static int outofwin(void);
static void rewindow(void);
static int newcol(unsigned char, int);
static void display(char *, char *, Wahr);
static void ed_mov_opt(int, char *);
static int expand_word(int);
static int complete_word(int, int);
static int print_expansions(struct edstate *, int);
static void vi_error(void);
static void vi_macro_reset(void);
static int x_vi_putbuf(const char *, size_t);
#define char_len(c) (ksh_isctrl(c) ? 2 : 1)
#define ksh_unctrl(c) asc2rtt(rtt2asc(c) ^ 0x40U)

#define vC	0x01		/* a valid command that isn't a vM, vE, vU */
#define vM	0x02		/* movement command (h, l, etc.) */
#define vE	0x04		/* extended command (c, d, y) */
#define vX	0x08		/* long command (@, f, F, t, T, etc.) */
#define vU	0x10		/* an UN-undoable command (that isn't a vM) */
#define vB	0x20		/* bad command (^@) */
#define vZ	0x40		/* repeat count defaults to 0 (not 1) */
#define vS	0x80		/* search (/, ?) */

#define is_bad(c)	(classify[rtt2asc(c) & 0x7F] & vB)
#define is_cmd(c)	(classify[rtt2asc(c) & 0x7F] & (vM | vE | vC | vU))
#define is_move(c)	(classify[rtt2asc(c) & 0x7F] & vM)
#define is_extend(c)	(classify[rtt2asc(c) & 0x7F] & vE)
#define is_long(c)	(classify[rtt2asc(c) & 0x7F] & vX)
#define is_undoable(c)	(!(classify[rtt2asc(c) & 0x7F] & vU))
#define is_srch(c)	(classify[rtt2asc(c) & 0x7F] & vS)
#define is_zerocount(c)	(classify[rtt2asc(c) & 0x7F] & vZ)

static const unsigned char classify[128] = {
/*	 0	1	2	3	4	5	6	7	*/
/* 0	^@	^A	^B	^C	^D	^E	^F	^G	*/
	vB,	0,	0,	0,	0,	vC|vU,	vC|vZ,	0,
/* 1	^H	^I	^J	^K	^L	^M	^N	^O	*/
	vM,	vC|vZ,	0,	0,	vC|vU,	0,	vC,	0,
/* 2	^P	^Q	^R	^S	^T	^U	^V	^W	*/
	vC,	0,	vC|vU,	0,	0,	0,	vC,	0,
/* 3	^X	^Y	^Z	^[	^\	^]	^^	^_	*/
	vC,	0,	0,	vC|vZ,	0,	0,	0,	0,
/* 4	<space>	!	"	#	$	%	&	'	*/
	vM,	0,	0,	vC,	vM,	vM,	0,	0,
/* 5	(	)	*	+	,	-	.	/	*/
	0,	0,	vC,	vC,	vM,	vC,	0,	vC|vS,
/* 6	0	1	2	3	4	5	6	7	*/
	vM,	0,	0,	0,	0,	0,	0,	0,
/* 7	8	9	:	;	<	=	>	?	*/
	0,	0,	0,	vM,	0,	vC,	0,	vC|vS,
/* 8	@	A	B	C	D	E	F	G	*/
	vC|vX,	vC,	vM,	vC,	vC,	vM,	vM|vX,	vC|vU|vZ,
/* 9	H	I	J	K	L	M	N	O	*/
	0,	vC,	0,	0,	0,	0,	vC|vU,	vU,
/* A	P	Q	R	S	T	U	V	W	*/
	vC,	0,	vC,	vC,	vM|vX,	vC,	0,	vM,
/* B	X	Y	Z	[	\	]	^	_	*/
	vC,	vC|vU,	0,	vU,	vC|vZ,	0,	vM,	vC|vZ,
/* C	`	a	b	c	d	e	f	g	*/
	0,	vC,	vM,	vE,	vE,	vM,	vM|vX,	vC|vZ,
/* D	h	i	j	k	l	m	n	o	*/
	vM,	vC,	vC|vU,	vC|vU,	vM,	0,	vC|vU,	0,
/* E	p	q	r	s	t	u	v	w	*/
	vC,	0,	vX,	vC,	vM|vX,	vC|vU,	vC|vU|vZ, vM,
/* F	x	y	z	{	|	}	~	^?	*/
	vC,	vE|vU,	0,	0,	vM|vZ,	0,	vC,	0
};

#define MAXVICMD	3
#define SRCHLEN		40

#define INSERT		1
#define REPLACE		2

#define VNORMAL		0		/* command, insert or replace mode */
#define VARG1		1		/* digit prefix (first, eg, 5l) */
#define VEXTCMD		2		/* cmd + movement (eg, cl) */
#define VARG2		3		/* digit prefix (second, eg, 2c3l) */
#define VXCH		4		/* f, F, t, T, @ */
#define VFAIL		5		/* bad command */
#define VCMD		6		/* single char command (eg, X) */
#define VREDO		7		/* . */
#define VLIT		8		/* ^V */
#define VSEARCH		9		/* /, ? */
#define VVERSION	10		/* <ESC> ^V */
#define VPREFIX2	11		/* ^[[ and ^[O in insert mode */

static struct edstate	*save_edstate(struct edstate *old);
static void		restore_edstate(struct edstate *old, struct edstate *news);
static void		free_edstate(struct edstate *old);

static struct edstate	ebuf;
static struct edstate	undobuf;

static struct edstate	*vs;		/* current Vi editing mode state */
static struct edstate	*undo;

static char *ibuf;			/* input buffer */
static Wahr first_insert;		/* set when starting in insert mode */
static int saved_inslen;		/* saved inslen for first insert */
static int inslen;			/* length of input buffer */
static int srchlen;			/* length of current search pattern */
static char *ybuf;			/* yank buffer */
static int yanklen;			/* length of yank buffer */
static kby fsavecmd = ORD(' ');		/* last find command */
static int fsavech;			/* character to find */
static char lastcmd[MAXVICMD];		/* last non-move command */
static int lastac;			/* argcnt for lastcmd */
static kby lastsearch = ORD(' ');	/* last search command */
static char srchpat[SRCHLEN];		/* last search pattern */
static int insert;			/* <>0 in insert mode */
static int hnum;			/* position in history */
static int ohnum;			/* history line copied (after mod) */
static int hlast;			/* 1 past last position in history */
static int state;

/*
 * Information for keeping track of macros that are being expanded.
 * The format of buf is the alias contents followed by a NUL byte followed
 * by the name (letter) of the alias. The end of the buffer is marked by
 * a double NUL. The name of the alias is stored so recursive macros can
 * be detected.
 */
struct macro_state {
	unsigned char *p;	/* current position in buf */
	unsigned char *buf;	/* pointer to macro(s) being expanded */
	size_t len;		/* how much data in buffer */
};
static struct macro_state macro;

/* last input was expanded */
static enum expand_mode {
	NONE = 0, EXPAND, COMPLETE, PRINT
} expanded;

static ssize_t
x_vi(char *buf)
{
	int c;

	state = VNORMAL;
	ohnum = hnum = hlast = histnum(-1) + 1;
	insert = INSERT;
	saved_inslen = inslen;
	first_insert = Ja;
	inslen = 0;
	vi_macro_reset();

	ebuf.cbuf = buf;
	if (undobuf.cbuf == NULL) {
		ibuf = alloc(LINE, AEDIT);
		ybuf = alloc(LINE, AEDIT);
		undobuf.cbuf = alloc(LINE, AEDIT);
	}
	undobuf.cbufsize = ebuf.cbufsize = LINE;
	undobuf.linelen = ebuf.linelen = 0;
	undobuf.cursor = ebuf.cursor = 0;
	undobuf.winleft = ebuf.winleft = 0;
	vs = &ebuf;
	undo = &undobuf;

	x_init_prompt(Ja);
	x_col = pwidth;

	if (wbuf_len != x_cols - 3 && ((wbuf_len = x_cols - 3))) {
		wbuf[0] = aresize(wbuf[0], wbuf_len, AEDIT);
		wbuf[1] = aresize(wbuf[1], wbuf_len, AEDIT);
	}
	if (wbuf_len) {
		memset(wbuf[0], ' ', wbuf_len);
		memset(wbuf[1], ' ', wbuf_len);
	}
	winwidth = x_cols - pwidth - 3;
	win = 0;
	morec = ' ';
	holdlen = 0;

	editmode = 2;
	x_flush();
	while (/* CONSTCOND */ 1) {
		if (macro.p) {
			c = (unsigned char)*macro.p++;
			/* end of current macro? */
			if (!c) {
				/* more macros left to finish? */
				if (*macro.p++)
					continue;
				/* must be the end of all the macros */
				vi_macro_reset();
				c = x_getc();
			}
		} else
			c = x_getc();

		if (c == -1)
			break;
		if (state != VLIT) {
			if (isched(c, edchars.intr) ||
			    isched(c, edchars.quit)) {
				/* shove input buffer away */
				xbuf = ebuf.cbuf;
				xep = xbuf;
				if (ebuf.linelen > 0)
					xep += ebuf.linelen;
				/* pretend we got an interrupt */
				x_intr(isched(c, edchars.intr) ?
				    SIGINT : SIGQUIT, c);
			} else if (isched(c, edchars.eof) &&
			    state != VVERSION) {
				if (vs->linelen == 0) {
					if (ksh_isctrl(c)) {
						x_putc('^');
						c = ksh_unctrl(c);
					}
					x_putc(c);
					c = -1;
					break;
				}
				continue;
			}
		}
		if (vi_hook(c))
			break;
		x_flush();
	}

	x_putc('\r');
	x_putc('\n');
	x_flush();

	if (c == -1 || (ssize_t)LINE <= vs->linelen)
		return (-1);

	if (vs->cbuf != buf)
		memcpy(buf, vs->cbuf, vs->linelen);

	buf[vs->linelen++] = '\n';

	return (vs->linelen);
}

static int
vi_hook(int ch)
{
	static char curcmd[MAXVICMD], locpat[SRCHLEN];
	static int cmdlen, argc1, argc2;

	switch (state) {

	case VNORMAL:
		/* PC scancodes */
		if (!ch) {
			cmdlen = 0;
			switch (ch = x_getc()) {
			case 71: ch = ORD('0'); goto pseudo_vi_command;
			case 72: ch = ORD('k'); goto pseudo_vi_command;
			case 73: ch = ORD('A'); goto vi_xfunc_search;
			case 75: ch = ORD('h'); goto pseudo_vi_command;
			case 77: ch = ORD('l'); goto pseudo_vi_command;
			case 79: ch = ORD('$'); goto pseudo_vi_command;
			case 80: ch = ORD('j'); goto pseudo_vi_command;
			case 81: ch = ORD('B'); goto vi_xfunc_search;
			case 83: ch = ORD('x'); goto pseudo_vi_command;
			default: ch = 0; goto vi_insert_failed;
			}
		}
		if (insert != 0) {
			if (ch == CTRL_V) {
				state = VLIT;
				ch = ORD('^');
			}
			switch (vi_insert(ch)) {
			case -1:
 vi_insert_failed:
				vi_error();
				state = VNORMAL;
				break;
			case 0:
				if (state == VLIT) {
					vs->cursor--;
					refresh(Nee);
				} else
					refresh(insert != 0);
				break;
			case 1:
				return (1);
			}
		} else {
			if (ctype(ch, C_CR | C_LF))
				return (1);
			cmdlen = 0;
			argc1 = 0;
			if (ctype(ch, C_DIGIT) && ord(ch) != ORD('0')) {
				argc1 = ksh_numdig(ch);
				state = VARG1;
			} else {
 pseudo_vi_command:
				curcmd[cmdlen++] = ch;
				state = nextstate(ch);
				if (state == VSEARCH) {
					save_cbuf();
					vs->cursor = 0;
					vs->linelen = 0;
					if (putbuf(ord(ch) == ORD('/') ?
					    "/" : "?", 1, Nee) != 0)
						return (-1);
					refresh(Nee);
				}
				if (state == VVERSION) {
					save_cbuf();
					vs->cursor = 0;
					vs->linelen = 0;
					putbuf(KSH_VERSION,
					    strlen(KSH_VERSION), Nee);
					refresh(Nee);
				}
			}
		}
		break;

	case VLIT:
		if (is_bad(ch)) {
			del_range(vs->cursor, vs->cursor + 1);
			vi_error();
		} else
			vs->cbuf[vs->cursor++] = ch;
		refresh(Ja);
		state = VNORMAL;
		break;

	case VVERSION:
		restore_cbuf();
		state = VNORMAL;
		refresh(Nee);
		break;

	case VARG1:
		if (ctype(ch, C_DIGIT))
			argc1 = argc1 * 10 + ksh_numdig(ch);
		else {
			curcmd[cmdlen++] = ch;
			state = nextstate(ch);
		}
		break;

	case VEXTCMD:
		argc2 = 0;
		if (ctype(ch, C_DIGIT) && ord(ch) != ORD('0')) {
			argc2 = ksh_numdig(ch);
			state = VARG2;
			return (0);
		} else {
			curcmd[cmdlen++] = ch;
			if (ch == curcmd[0])
				state = VCMD;
			else if (is_move(ch))
				state = nextstate(ch);
			else
				state = VFAIL;
		}
		break;

	case VARG2:
		if (ctype(ch, C_DIGIT))
			argc2 = argc2 * 10 + ksh_numdig(ch);
		else {
			if (argc1 == 0)
				argc1 = argc2;
			else
				argc1 *= argc2;
			curcmd[cmdlen++] = ch;
			if (ch == curcmd[0])
				state = VCMD;
			else if (is_move(ch))
				state = nextstate(ch);
			else
				state = VFAIL;
		}
		break;

	case VXCH:
		if (ch == CTRL_BO)
			state = VNORMAL;
		else {
			curcmd[cmdlen++] = ch;
			state = VCMD;
		}
		break;

	case VSEARCH:
		if (ctype(ch, C_CR | C_LF) /* || ch == CTRL_BO */ ) {
			restore_cbuf();
			/* Repeat last search? */
			if (srchlen == 0) {
				if (!srchpat[0]) {
					vi_error();
					state = VNORMAL;
					refresh(Nee);
					return (0);
				}
			} else {
				locpat[srchlen] = '\0';
				memcpy(srchpat, locpat, srchlen + 1);
			}
			state = VCMD;
		} else if (isched(ch, edchars.erase) || ch == CTRL_H) {
			if (srchlen != 0) {
				srchlen--;
				vs->linelen -= char_len(locpat[srchlen]);
				vs->cursor = vs->linelen;
				refresh(Nee);
				return (0);
			}
			restore_cbuf();
			state = VNORMAL;
			refresh(Nee);
		} else if (isched(ch, edchars.kill)) {
			srchlen = 0;
			vs->linelen = 1;
			vs->cursor = 1;
			refresh(Nee);
			return (0);
		} else if (isched(ch, edchars.werase)) {
			unsigned int i, n;
			struct edstate new_es, *save_es;

			new_es.cursor = srchlen;
			new_es.cbuf = locpat;

			save_es = vs;
			vs = &new_es;
			n = backword(1);
			vs = save_es;

			i = (unsigned)srchlen;
			while (i-- > n)
				vs->linelen -= char_len(locpat[i]);
			srchlen = (int)n;
			vs->cursor = vs->linelen;
			refresh(Nee);
			return (0);
		} else {
			if (srchlen == SRCHLEN - 1)
				vi_error();
			else {
				locpat[srchlen++] = ch;
				if (ksh_isctrl(ch)) {
					if ((size_t)vs->linelen + 2 >
					    (size_t)vs->cbufsize)
						vi_error();
					vs->cbuf[vs->linelen++] = '^';
					vs->cbuf[vs->linelen++] = ksh_unctrl(ch);
				} else {
					if (vs->linelen >= vs->cbufsize)
						vi_error();
					vs->cbuf[vs->linelen++] = ch;
				}
				vs->cursor = vs->linelen;
				refresh(Nee);
			}
			return (0);
		}
		break;

	case VPREFIX2:
 vi_xfunc_search:
		state = VFAIL;
		switch (ch) {
		case ORD('A'):
		case ORD('B'):
			/* the cursor may not be at the BOL */
			if (!vs->cursor)
				break;
			/* nor further in the line than we can search for */
			if ((size_t)vs->cursor >= sizeof(srchpat) - 1)
				vs->cursor = sizeof(srchpat) - 2;
			/* anchor the search pattern */
			srchpat[0] = '^';
			/* take current line up to the cursor */
			memcpy(srchpat + 1, vs->cbuf, vs->cursor);
			srchpat[vs->cursor + 1] = '\0';
			/* set a magic flag */
			argc1 = 2 + (int)vs->cursor;
			/* and emulate a history search */
			/* search backwards if PgUp, forwards for PgDn */
			lastsearch = ch == ORD('A') ? '/' : '?';
			*curcmd = 'n';
			goto pseudo_VCMD;
		}
		break;
	}

	switch (state) {
	case VCMD:
 pseudo_VCMD:
		state = VNORMAL;
		switch (vi_cmd(argc1, curcmd)) {
		case -1:
			vi_error();
			refresh(Nee);
			break;
		case 0:
			if (insert != 0)
				inslen = 0;
			refresh(insert != 0);
			break;
		case 1:
			refresh(Nee);
			return (1);
		case 2:
			/* back from a 'v' command - don't redraw the screen */
			return (1);
		}
		break;

	case VREDO:
		state = VNORMAL;
		if (argc1 != 0)
			lastac = argc1;
		switch (vi_cmd(lastac, lastcmd)) {
		case -1:
			vi_error();
			refresh(Nee);
			break;
		case 0:
			if (insert != 0) {
				if (lastcmd[0] == 's' ||
				    isCh(lastcmd[0], 'C', 'c')) {
					if (redo_insert(1) != 0)
						vi_error();
				} else {
					if (redo_insert(lastac) != 0)
						vi_error();
				}
			}
			refresh(Nee);
			break;
		case 1:
			refresh(Nee);
			return (1);
		case 2:
			/* back from a 'v' command - can't happen */
			break;
		}
		break;

	case VFAIL:
		state = VNORMAL;
		vi_error();
		break;
	}
	return (0);
}

static int
nextstate(int ch)
{
	if (is_extend(ch))
		return (VEXTCMD);
	else if (is_srch(ch))
		return (VSEARCH);
	else if (is_long(ch))
		return (VXCH);
	else if (ch == '.')
		return (VREDO);
	else if (ch == CTRL_V)
		return (VVERSION);
	else if (is_cmd(ch))
		return (VCMD);
	else
		return (VFAIL);
}

static int
vi_insert(int ch)
{
	int tcursor;

	if (isched(ch, edchars.erase) || ch == CTRL_H) {
		if (insert == REPLACE) {
			if (vs->cursor == undo->cursor) {
				vi_error();
				return (0);
			}
			if (inslen > 0)
				inslen--;
			vs->cursor--;
			if (vs->cursor >= undo->linelen)
				vs->linelen--;
			else
				vs->cbuf[vs->cursor] = undo->cbuf[vs->cursor];
		} else {
			if (vs->cursor == 0)
				return (0);
			if (inslen > 0)
				inslen--;
			vs->cursor--;
			vs->linelen--;
			memmove(&vs->cbuf[vs->cursor], &vs->cbuf[vs->cursor + 1],
			    vs->linelen - vs->cursor + 1);
		}
		expanded = NONE;
		return (0);
	}
	if (isched(ch, edchars.kill)) {
		if (vs->cursor != 0) {
			inslen = 0;
			memmove(vs->cbuf, &vs->cbuf[vs->cursor],
			    vs->linelen - vs->cursor);
			vs->linelen -= vs->cursor;
			vs->cursor = 0;
		}
		expanded = NONE;
		return (0);
	}
	if (isched(ch, edchars.werase)) {
		if (vs->cursor != 0) {
			tcursor = backword(1);
			memmove(&vs->cbuf[tcursor], &vs->cbuf[vs->cursor],
			    vs->linelen - vs->cursor);
			vs->linelen -= vs->cursor - tcursor;
			if (inslen < vs->cursor - tcursor)
				inslen = 0;
			else
				inslen -= vs->cursor - tcursor;
			vs->cursor = tcursor;
		}
		expanded = NONE;
		return (0);
	}
	/*
	 * If any chars are entered before escape, trash the saved insert
	 * buffer (if user inserts & deletes char, ibuf gets trashed and
	 * we don't want to use it)
	 */
	if (first_insert && ch != CTRL_BO)
		saved_inslen = 0;
	switch (ch) {
	case '\0':
		return (-1);

	case '\r':
	case '\n':
		return (1);

	case CTRL_BO:
		expanded = NONE;
		if (first_insert) {
			first_insert = Nee;
			if (inslen == 0) {
				inslen = saved_inslen;
				return (redo_insert(0));
			}
			lastcmd[0] = 'a';
			lastac = 1;
		}
		if (lastcmd[0] == 's' || isCh(lastcmd[0], 'C', 'c'))
			return (redo_insert(0));
		else
			return (redo_insert(lastac - 1));

	/* { start nonstandard vi commands */
	case CTRL_X:
		expand_word(0);
		break;

	case CTRL_F:
		complete_word(0, 0);
		break;

	case CTRL_E:
		print_expansions(vs, 0);
		break;

	case CTRL_I:
		if (Flag(FVITABCOMPLETE)) {
			complete_word(0, 0);
			break;
		}
		/* FALLTHROUGH */
	/* end nonstandard vi commands } */

	default:
		if (vs->linelen >= vs->cbufsize - 1)
			return (-1);
		ibuf[inslen++] = ch;
		if (insert == INSERT) {
			memmove(&vs->cbuf[vs->cursor + 1], &vs->cbuf[vs->cursor],
			    vs->linelen - vs->cursor);
			vs->linelen++;
		}
		vs->cbuf[vs->cursor++] = ch;
		if (insert == REPLACE && vs->cursor > vs->linelen)
			vs->linelen++;
		expanded = NONE;
	}
	return (0);
}

static int
vi_cmd(int argcnt, const char *cmd)
{
	int ncursor;
	int cur, c1, c2;
	int any;
	Wahr b;
	struct edstate *t;

	if (argcnt == 0 && !is_zerocount(*cmd))
		argcnt = 1;

	if (is_move(*cmd)) {
		if ((cur = domove(argcnt, cmd, 0)) >= 0) {
			if (cur == vs->linelen && cur != 0)
				cur--;
			vs->cursor = cur;
		} else
			return (-1);
	} else {
		/* Don't save state in middle of macro.. */
		if (is_undoable(*cmd) && !macro.p) {
			undo->winleft = vs->winleft;
			memmove(undo->cbuf, vs->cbuf, vs->linelen);
			undo->linelen = vs->linelen;
			undo->cursor = vs->cursor;
			lastac = argcnt;
			memmove(lastcmd, cmd, MAXVICMD);
		}
		switch (ord(*cmd)) {

		case CTRL_L:
		case CTRL_R:
			redraw_line(Ja);
			break;

		case ORD('@'):
			{
				static char alias[] = "_\0";
				struct tbl *ap;
				size_t olen, nlen;
				char *p, *nbuf;

				/* lookup letter in alias list... */
				alias[1] = cmd[1];
				ap = ktsearch(&aliases, alias, hash(alias));
				if (!cmd[1] || !ap || !(ap->flag & ISSET))
					return (-1);
				/* check if this is a recursive call... */
				if ((p = (char *)macro.p))
					while ((p = strnul(p)) && p[1])
						if (*++p == cmd[1])
							return (-1);
				/* insert alias into macro buffer */
				nlen = strlen(ap->val.s) + 1U;
				olen = !macro.p ? 2 :
				    macro.len - (macro.p - macro.buf);
				nbuf = alloc1(nlen + 1U, olen, AEDIT);
				memcpy(nbuf, ap->val.s, nlen);
				nbuf[nlen++] = cmd[1];
				if (macro.p) {
					memcpy(nbuf + nlen, macro.p, olen);
					afree(macro.buf, AEDIT);
					nlen += olen;
				} else {
					nbuf[nlen++] = '\0';
					nbuf[nlen++] = '\0';
				}
				macro.p = macro.buf = (unsigned char *)nbuf;
				macro.len = nlen;
			}
			break;

		case ORD('a'):
			modified = 1;
			hnum = hlast;
			if (vs->linelen != 0)
				vs->cursor++;
			insert = INSERT;
			break;

		case ORD('A'):
			modified = 1;
			hnum = hlast;
			del_range(0, 0);
			vs->cursor = vs->linelen;
			insert = INSERT;
			break;

		case ORD('S'):
			vs->cursor = domovebeg();
			del_range(vs->cursor, vs->linelen);
			modified = 1;
			hnum = hlast;
			insert = INSERT;
			break;

		case ORD('Y'):
			cmd = "y$";
			/* ahhhhhh... */

			/* FALLTHROUGH */
		case ORD('c'):
		case ORD('d'):
		case ORD('y'):
			if (*cmd == cmd[1]) {
				c1 = *cmd == 'c' ? domovebeg() : 0;
				c2 = vs->linelen;
			} else if (!is_move(cmd[1]))
				return (-1);
			else {
				if ((ncursor = domove(argcnt, &cmd[1], 1)) < 0)
					return (-1);
				if (*cmd == 'c' && isCh(cmd[1], 'W', 'w') &&
				    !ctype(vs->cbuf[vs->cursor], C_SPACE)) {
					do {
						--ncursor;
					} while (ctype(vs->cbuf[ncursor], C_SPACE));
					ncursor++;
				}
				if (ncursor > vs->cursor) {
					c1 = vs->cursor;
					c2 = ncursor;
				} else {
					c1 = ncursor;
					c2 = vs->cursor;
					if (cmd[1] == '%')
						c2++;
				}
			}
			if (*cmd != 'c' && c1 != c2)
				yank_range(c1, c2);
			if (*cmd != 'y') {
				del_range(c1, c2);
				vs->cursor = c1;
			}
			if (*cmd == 'c') {
				modified = 1;
				hnum = hlast;
				insert = INSERT;
			}
			break;

		case ORD('p'):
			modified = 1;
			hnum = hlast;
			if (vs->linelen != 0)
				vs->cursor++;
			while (putbuf(ybuf, yanklen, Nee) == 0 &&
			    --argcnt > 0)
				;
			if (vs->cursor != 0)
				vs->cursor--;
			if (argcnt != 0)
				return (-1);
			break;

		case ORD('P'):
			modified = 1;
			hnum = hlast;
			any = 0;
			while (putbuf(ybuf, yanklen, Nee) == 0 &&
			    --argcnt > 0)
				any = 1;
			if (any && vs->cursor != 0)
				vs->cursor--;
			if (argcnt != 0)
				return (-1);
			break;

		case ORD('C'):
			modified = 1;
			hnum = hlast;
			del_range(vs->cursor, vs->linelen);
			insert = INSERT;
			break;

		case ORD('D'):
			yank_range(vs->cursor, vs->linelen);
			del_range(vs->cursor, vs->linelen);
			if (vs->cursor != 0)
				vs->cursor--;
			break;

		case ORD('g'):
			if (!argcnt)
				argcnt = hlast;
			/* FALLTHROUGH */
		case ORD('G'):
			if (!argcnt)
				argcnt = 1;
			else
				argcnt = hlast - (source->line - argcnt);
			if (grabhist(modified, argcnt - 1) < 0)
				return (-1);
			else {
				modified = 0;
				hnum = argcnt - 1;
			}
			break;

		case ORD('i'):
			modified = 1;
			hnum = hlast;
			insert = INSERT;
			break;

		case ORD('I'):
			modified = 1;
			hnum = hlast;
			vs->cursor = domovebeg();
			insert = INSERT;
			break;

		case ORD('j'):
		case ORD('+'):
		case CTRL_N:
			if (grabhist(modified, hnum + argcnt) < 0)
				return (-1);
			else {
				modified = 0;
				hnum += argcnt;
			}
			break;

		case ORD('k'):
		case ORD('-'):
		case CTRL_P:
			if (grabhist(modified, hnum - argcnt) < 0)
				return (-1);
			else {
				modified = 0;
				hnum -= argcnt;
			}
			break;

		case ORD('r'):
			if (vs->linelen == 0)
				return (-1);
			modified = 1;
			hnum = hlast;
			if (cmd[1] == 0)
				vi_error();
			else {
				int n;

				if (vs->cursor + argcnt > vs->linelen)
					return (-1);
				for (n = 0; n < argcnt; ++n)
					vs->cbuf[vs->cursor + n] = cmd[1];
				vs->cursor += n - 1;
			}
			break;

		case ORD('R'):
			modified = 1;
			hnum = hlast;
			insert = REPLACE;
			break;

		case ORD('s'):
			if (vs->linelen == 0)
				return (-1);
			modified = 1;
			hnum = hlast;
			if (vs->cursor + argcnt > vs->linelen)
				argcnt = vs->linelen - vs->cursor;
			del_range(vs->cursor, vs->cursor + argcnt);
			insert = INSERT;
			break;

		case ORD('v'):
			if (!argcnt) {
				if (modified) {
					vs->cbuf[vs->linelen] = '\0';
					histsave(&source->line, vs->cbuf,
					    HIST_STORE, Ja);
				} else
					argcnt = source->line + 1 -
					    (hlast - hnum);
			}
			if (argcnt)
				shf_snprintf(vs->cbuf, vs->cbufsize, Tf_sd,
				    ctrl_x_e, argcnt);
			else
				strlcpy(vs->cbuf, ctrl_x_e, vs->cbufsize);
			vs->linelen = strlen(vs->cbuf);
			return (2);

		case ORD('x'):
			if (vs->linelen == 0)
				return (-1);
			modified = 1;
			hnum = hlast;
			if (vs->cursor + argcnt > vs->linelen)
				argcnt = vs->linelen - vs->cursor;
			yank_range(vs->cursor, vs->cursor + argcnt);
			del_range(vs->cursor, vs->cursor + argcnt);
			break;

		case ORD('X'):
			if (vs->cursor > 0) {
				modified = 1;
				hnum = hlast;
				if (vs->cursor < argcnt)
					argcnt = vs->cursor;
				yank_range(vs->cursor - argcnt, vs->cursor);
				del_range(vs->cursor - argcnt, vs->cursor);
				vs->cursor -= argcnt;
			} else
				return (-1);
			break;

		case ORD('u'):
			t = vs;
			vs = undo;
			undo = t;
			break;

		case ORD('U'):
			if (!modified)
				return (-1);
			if (grabhist(modified, ohnum) < 0)
				return (-1);
			modified = 0;
			hnum = ohnum;
			break;

		case ORD('?'):
			if (hnum == hlast)
				hnum = -1;
			/* ahhh */

			/* FALLTHROUGH */
		case ORD('/'):
			c1 = 1;
			srchlen = 0;
			lastsearch = *cmd;
			if (0)
				/* FALLTHROUGH */
		case ORD('n'):
		case ORD('N'):
			  c1 = 0;
			if (lastsearch == ORD(' '))
				return (-1);
			b = (lastsearch == ORD('?'));
			if (*cmd == 'N')
				b = !b;
			if ((c2 = grabsearch(srchpat, modified, hnum, b)) < 0) {
				if (c1) {
					restore_cbuf();
					refresh(Nee);
				}
				return (-1);
			} else {
				modified = 0;
				hnum = c2;
				ohnum = hnum;
			}
			if (argcnt >= 2) {
				/* flag from cursor-up command */
				vs->cursor = argcnt - 2;
				return (0);
			}
			break;
		case ORD('_'):
			{
				Wahr inspace;
				char *p, *sp;

				if (histnum(-1) < 0)
					return (-1);
				p = *histpos();
				if (argcnt) {
					while (ctype(*p, C_SPACE))
						p++;
					while (*p && --argcnt) {
						while (*p && !ctype(*p, C_SPACE))
							p++;
						while (ctype(*p, C_SPACE))
							p++;
					}
					if (!*p)
						return (-1);
					sp = p;
				} else {
					sp = p;
					inspace = Nee;
					while (*p) {
						if (ctype(*p, C_SPACE))
							inspace = Ja;
						else if (inspace) {
							inspace = Nee;
							sp = p;
						}
						p++;
					}
					p = sp;
				}
				modified = 1;
				hnum = hlast;
				if (vs->cursor != vs->linelen)
					vs->cursor++;
				while (*p && !ctype(*p, C_SPACE)) {
					argcnt++;
					p++;
				}
				if (putbuf(T1space, 1, Nee) != 0 ||
				    putbuf(sp, argcnt, Nee) != 0) {
					if (vs->cursor != 0)
						vs->cursor--;
					return (-1);
				}
				insert = INSERT;
			}
			break;

		case ORD('~'):
			{
				char *p;
				int i;

				if (vs->linelen == 0)
					return (-1);
				for (i = 0; i < argcnt; i++) {
					p = &vs->cbuf[vs->cursor];
					if (ctype(*p, C_LOWER)) {
						modified = 1;
						hnum = hlast;
						*p = ksh_toupper(*p);
					} else if (ctype(*p, C_UPPER)) {
						modified = 1;
						hnum = hlast;
						*p = ksh_tolower(*p);
					}
					if (vs->cursor < vs->linelen - 1)
						vs->cursor++;
				}
				break;
			}

		case ORD('#'):
			{
				int ret = x_do_comment(vs->cbuf, vs->cbufsize,
				    &vs->linelen);
				if (ret >= 0)
					vs->cursor = 0;
				return (ret);
			}

		/* AT&T ksh */
		case ORD('='):
		/* Nonstandard vi/ksh */
		case CTRL_E:
			print_expansions(vs, 1);
			break;

		/* Nonstandard vi/ksh */
		case CTRL_I:
			if (!Flag(FVITABCOMPLETE))
				return (-1);
			complete_word(1, argcnt);
			break;

		/* some annoying AT&T kshs */
		case CTRL_BO:
			if (!Flag(FVIESCCOMPLETE))
				return (-1);
			/* FALLTHROUGH */
		/* AT&T ksh */
		case ORD('\\'):
		/* Nonstandard vi/ksh */
		case CTRL_F:
			complete_word(1, argcnt);
			break;

		/* AT&T ksh */
		case ORD('*'):
		/* Nonstandard vi/ksh */
		case CTRL_X:
			expand_word(1);
			break;

		/* mksh: cursor movement */
		case ORD('['):
		case ORD('O'):
			state = VPREFIX2;
			if (vs->linelen != 0)
				vs->cursor++;
			insert = INSERT;
			return (0);
		}
		if (insert == 0 && vs->cursor != 0 && vs->cursor >= vs->linelen)
			vs->cursor--;
	}
	return (0);
}

static int
domove(int argcnt, const char *cmd, int sub)
{
	int ncursor = 0, i = 0, t;
	unsigned int bcount;

	switch (ord(*cmd)) {
	case ORD('b'):
		if (!sub && vs->cursor == 0)
			return (-1);
		ncursor = backword(argcnt);
		break;

	case ORD('B'):
		if (!sub && vs->cursor == 0)
			return (-1);
		ncursor = Backword(argcnt);
		break;

	case ORD('e'):
		if (!sub && vs->cursor + 1 >= vs->linelen)
			return (-1);
		ncursor = endword(argcnt);
		if (sub && ncursor < vs->linelen)
			ncursor++;
		break;

	case ORD('E'):
		if (!sub && vs->cursor + 1 >= vs->linelen)
			return (-1);
		ncursor = Endword(argcnt);
		if (sub && ncursor < vs->linelen)
			ncursor++;
		break;

	case ORD('f'):
	case ORD('F'):
	case ORD('t'):
	case ORD('T'):
		fsavecmd = *cmd;
		fsavech = cmd[1];
		/* FALLTHROUGH */
	case ORD(','):
	case ORD(';'):
		if (fsavecmd == ORD(' '))
			return (-1);
		i = isCh(fsavecmd, 'F', 'f');
		t = rtt2asc(fsavecmd) > rtt2asc('a');
		if (*cmd == ',')
			t = !t;
		if ((ncursor = findch(fsavech, argcnt, isWahr(t),
		    isWahr(i))) < 0)
			return (-1);
		if (sub && t)
			ncursor++;
		break;

	case ORD('h'):
	case CTRL_H:
		if (!sub && vs->cursor == 0)
			return (-1);
		ncursor = vs->cursor - argcnt;
		if (ncursor < 0)
			ncursor = 0;
		break;

	case ORD(' '):
	case ORD('l'):
		if (!sub && vs->cursor + 1 >= vs->linelen)
			return (-1);
		if (vs->linelen != 0) {
			ncursor = vs->cursor + argcnt;
			if (ncursor > vs->linelen)
				ncursor = vs->linelen;
		}
		break;

	case ORD('w'):
		if (!sub && vs->cursor + 1 >= vs->linelen)
			return (-1);
		ncursor = forwword(argcnt);
		break;

	case ORD('W'):
		if (!sub && vs->cursor + 1 >= vs->linelen)
			return (-1);
		ncursor = Forwword(argcnt);
		break;

	case ORD('0'):
		ncursor = 0;
		break;

	case ORD('^'):
		ncursor = domovebeg();
		break;

	case ORD('|'):
		ncursor = argcnt;
		if (ncursor > vs->linelen)
			ncursor = vs->linelen;
		if (ncursor)
			ncursor--;
		break;

	case ORD('$'):
		if (vs->linelen != 0)
			ncursor = vs->linelen;
		else
			ncursor = 0;
		break;

	case ORD('%'):
		ncursor = vs->cursor;
		while (ncursor < vs->linelen &&
		    (i = bracktype(vs->cbuf[ncursor])) == 0)
			ncursor++;
		if (ncursor == vs->linelen)
			return (-1);
		bcount = 1;
		do {
			if (i > 0) {
				if (++ncursor >= vs->linelen)
					return (-1);
			} else {
				if (--ncursor < 0)
					return (-1);
			}
			t = bracktype(vs->cbuf[ncursor]);
			if (t == i)
				bcount++;
			else if (t == -i)
				bcount--;
		} while (bcount != 0);
		if (sub && i > 0)
			ncursor++;
		break;

	default:
		return (-1);
	}
	return (ncursor);
}

static int
domovebeg(void)
{
	int ncursor = 0;

	while (ncursor < vs->linelen - 1 &&
	    ctype(vs->cbuf[ncursor], C_SPACE))
		ncursor++;
	return (ncursor);
}

static int
redo_insert(int count)
{
	while (count-- > 0)
		if (putbuf(ibuf, inslen, isWahr(insert == REPLACE)) != 0)
			return (-1);
	if (vs->cursor > 0)
		vs->cursor--;
	insert = 0;
	return (0);
}

static void
yank_range(int a, int b)
{
	yanklen = b - a;
	if (yanklen != 0)
		memmove(ybuf, &vs->cbuf[a], yanklen);
}

static int
bracktype(int ch)
{
	switch (ord(ch)) {

	case ORD('('):
		return (1);

	case ORD('['):
		return (2);

	case ORD('{'):
		return (3);

	case ORD(')'):
		return (-1);

	case ORD(']'):
		return (-2);

	case ORD('}'):
		return (-3);

	default:
		return (0);
	}
}

/*
 *	Non user interface editor routines below here
 */

static void
save_cbuf(void)
{
	memmove(holdbufp, vs->cbuf, vs->linelen);
	holdlen = vs->linelen;
	holdbufp[holdlen] = '\0';
}

static void
restore_cbuf(void)
{
	vs->cursor = 0;
	vs->linelen = holdlen;
	memmove(vs->cbuf, holdbufp, holdlen);
}

/* return a new edstate */
static struct edstate *
save_edstate(struct edstate *old)
{
	struct edstate *news;

	news = alloc(sizeof(struct edstate), AEDIT);
	news->cbuf = alloc(old->cbufsize, AEDIT);
	memcpy(news->cbuf, old->cbuf, old->linelen);
	news->cbufsize = old->cbufsize;
	news->linelen = old->linelen;
	news->cursor = old->cursor;
	news->winleft = old->winleft;
	return (news);
}

static void
restore_edstate(struct edstate *news, struct edstate *old)
{
	memcpy(news->cbuf, old->cbuf, old->linelen);
	news->linelen = old->linelen;
	news->cursor = old->cursor;
	news->winleft = old->winleft;
	free_edstate(old);
}

static void
free_edstate(struct edstate *old)
{
	afree(old->cbuf, AEDIT);
	afree(old, AEDIT);
}

/*
 * this is used for calling x_escape() in complete_word()
 */
static int
x_vi_putbuf(const char *s, size_t len)
{
	return (putbuf(s, len, Nee));
}

static int
putbuf(const char *buf, ssize_t len, Wahr repl)
{
	if (len == 0)
		return (0);
	if (repl) {
		if (vs->cursor + len >= vs->cbufsize)
			return (-1);
		if (vs->cursor + len > vs->linelen)
			vs->linelen = vs->cursor + len;
	} else {
		if (vs->linelen + len >= vs->cbufsize)
			return (-1);
		memmove(&vs->cbuf[vs->cursor + len], &vs->cbuf[vs->cursor],
		    vs->linelen - vs->cursor);
		vs->linelen += len;
	}
	memmove(&vs->cbuf[vs->cursor], buf, len);
	vs->cursor += len;
	return (0);
}

static void
del_range(int a, int b)
{
	if (vs->linelen != b)
		memmove(&vs->cbuf[a], &vs->cbuf[b], vs->linelen - b);
	vs->linelen -= b - a;
}

static int
findch(int ch, int cnt, Wahr forw, Wahr incl)
{
	int ncursor;

	if (vs->linelen == 0)
		return (-1);
	ncursor = vs->cursor;
	while (cnt--) {
		do {
			if (forw) {
				if (++ncursor == vs->linelen)
					return (-1);
			} else {
				if (--ncursor < 0)
					return (-1);
			}
		} while (vs->cbuf[ncursor] != ch);
	}
	if (!incl) {
		if (forw)
			ncursor--;
		else
			ncursor++;
	}
	return (ncursor);
}

static int
forwword(int argcnt)
{
	int ncursor;

	ncursor = vs->cursor;
	while (ncursor < vs->linelen && argcnt--) {
		if (ctype(vs->cbuf[ncursor], C_ALNUX))
			while (ncursor < vs->linelen &&
			    ctype(vs->cbuf[ncursor], C_ALNUX))
				ncursor++;
		else if (!ctype(vs->cbuf[ncursor], C_SPACE))
			while (ncursor < vs->linelen &&
			    !ctype(vs->cbuf[ncursor], C_ALNUX | C_SPACE))
				ncursor++;
		while (ncursor < vs->linelen &&
		    ctype(vs->cbuf[ncursor], C_SPACE))
			ncursor++;
	}
	return (ncursor);
}

static int
backword(int argcnt)
{
	int ncursor;

	ncursor = vs->cursor;
	while (ncursor > 0 && argcnt--) {
		while (--ncursor > 0 && ctype(vs->cbuf[ncursor], C_SPACE))
			;
		if (ncursor > 0) {
			if (ctype(vs->cbuf[ncursor], C_ALNUX))
				while (--ncursor >= 0 &&
				    ctype(vs->cbuf[ncursor], C_ALNUX))
					;
			else
				while (--ncursor >= 0 &&
				    !ctype(vs->cbuf[ncursor], C_ALNUX | C_SPACE))
					;
			ncursor++;
		}
	}
	return (ncursor);
}

static int
endword(int argcnt)
{
	int ncursor;

	ncursor = vs->cursor;
	while (ncursor < vs->linelen && argcnt--) {
		while (++ncursor < vs->linelen - 1 &&
		    ctype(vs->cbuf[ncursor], C_SPACE))
			;
		if (ncursor < vs->linelen - 1) {
			if (ctype(vs->cbuf[ncursor], C_ALNUX))
				while (++ncursor < vs->linelen &&
				    ctype(vs->cbuf[ncursor], C_ALNUX))
					;
			else
				while (++ncursor < vs->linelen &&
				    !ctype(vs->cbuf[ncursor], C_ALNUX | C_SPACE))
					;
			ncursor--;
		}
	}
	return (ncursor);
}

static int
Forwword(int argcnt)
{
	int ncursor;

	ncursor = vs->cursor;
	while (ncursor < vs->linelen && argcnt--) {
		while (ncursor < vs->linelen &&
		    !ctype(vs->cbuf[ncursor], C_SPACE))
			ncursor++;
		while (ncursor < vs->linelen &&
		    ctype(vs->cbuf[ncursor], C_SPACE))
			ncursor++;
	}
	return (ncursor);
}

static int
Backword(int argcnt)
{
	int ncursor;

	ncursor = vs->cursor;
	while (ncursor > 0 && argcnt--) {
		while (--ncursor >= 0 && ctype(vs->cbuf[ncursor], C_SPACE))
			;
		while (ncursor >= 0 && !ctype(vs->cbuf[ncursor], C_SPACE))
			ncursor--;
		ncursor++;
	}
	return (ncursor);
}

static int
Endword(int argcnt)
{
	int ncursor;

	ncursor = vs->cursor;
	while (ncursor < vs->linelen - 1 && argcnt--) {
		while (++ncursor < vs->linelen - 1 &&
		    ctype(vs->cbuf[ncursor], C_SPACE))
			;
		if (ncursor < vs->linelen - 1) {
			while (++ncursor < vs->linelen &&
			    !ctype(vs->cbuf[ncursor], C_SPACE))
				;
			ncursor--;
		}
	}
	return (ncursor);
}

static int
grabhist(int save, int n)
{
	char *hptr;

	if (n < 0 || n > hlast)
		return (-1);
	if (n == hlast) {
		restore_cbuf();
		ohnum = n;
		return (0);
	}
	histnum(n);
	if ((hptr = *histpos()) == NULL) {
		kwarnf(KWF_INTERNAL | KWF_WARNING | KWF_ONEMSG | KWF_NOERRNO,
		    "grabhist: bad history array");
		return (-1);
	}
	if (save)
		save_cbuf();
	if ((vs->linelen = strlen(hptr)) >= vs->cbufsize)
		vs->linelen = vs->cbufsize - 1;
	memmove(vs->cbuf, hptr, vs->linelen);
	vs->cursor = 0;
	ohnum = n;
	return (0);
}

static int
grabsearch(const char *pat, int save, int start, Wahr fwd)
{
	char *hptr;
	int hist;
	Wahr anchored;

	if ((start == 0 && !fwd) || (start >= hlast - 1 && fwd))
		return (-1);
	if (fwd)
		start++;
	else
		start--;
	anchored = *pat == '^' ? (++pat, Ja) : Nee;
	if ((hist = findhist(start, pat, fwd, anchored)) < 0) {
		/* (start != 0 && fwd && match(holdbufp, pat) >= 0) */
		if (start != 0 && fwd && strcmp(holdbufp, pat) >= 0) {
			restore_cbuf();
			return (0);
		} else
			return (-1);
	}
	if (save)
		save_cbuf();
	histnum(hist);
	hptr = *histpos();
	if ((vs->linelen = strlen(hptr)) >= vs->cbufsize)
		vs->linelen = vs->cbufsize - 1;
	memmove(vs->cbuf, hptr, vs->linelen);
	vs->cursor = 0;
	return (hist);
}

static void
redraw_line(Wahr newl)
{
	if (wbuf_len)
		memset(wbuf[win], ' ', wbuf_len);
	if (newl) {
		x_putc('\r');
		x_putc('\n');
	}
	x_pprompt();
	morec = ' ';
}

static void
refresh(Wahr leftside)
{
	if (outofwin())
		rewindow();
	display(wbuf[1 - win], wbuf[win], leftside);
	win = 1 - win;
}

static int
outofwin(void)
{
	int cur, col;

	if (vs->cursor < vs->winleft)
		return (1);
	col = 0;
	cur = vs->winleft;
	while (cur < vs->cursor)
		col = newcol((unsigned char)vs->cbuf[cur++], col);
	if (col >= winwidth)
		return (1);
	return (0);
}

static void
rewindow(void)
{
	int tcur, tcol;
	int holdcur1, holdcol1;
	int holdcur2, holdcol2;

	holdcur1 = holdcur2 = tcur = 0;
	holdcol1 = holdcol2 = tcol = 0;
	while (tcur < vs->cursor) {
		if (tcol - holdcol2 > winwidth / 2) {
			holdcur1 = holdcur2;
			holdcol1 = holdcol2;
			holdcur2 = tcur;
			holdcol2 = tcol;
		}
		tcol = newcol((unsigned char)vs->cbuf[tcur++], tcol);
	}
	while (tcol - holdcol1 > winwidth / 2)
		holdcol1 = newcol((unsigned char)vs->cbuf[holdcur1++],
		    holdcol1);
	vs->winleft = holdcur1;
}

static int
newcol(unsigned char ch, int col)
{
	if (ch == '\t')
		return ((col | 7) + 1);
	return (col + char_len(ch));
}

static void
display(char *wb1, char *wb2, Wahr leftside)
{
	unsigned char ch;
	char *twb1, *twb2, mc;
	int cur, col, cnt;
	int ncol = 0;
	int moreright;

	col = 0;
	cur = vs->winleft;
	moreright = 0;
	twb1 = wb1;
	while (col < winwidth && cur < vs->linelen) {
		if (cur == vs->cursor && leftside)
			ncol = col + pwidth;
		if ((ch = vs->cbuf[cur]) == '\t')
			do {
				*twb1++ = ' ';
			} while (++col < winwidth && (col & 7) != 0);
		else if (col < winwidth) {
			if (ksh_isctrl(ch)) {
				*twb1++ = '^';
				if (++col < winwidth) {
					*twb1++ = ksh_unctrl(ch);
					col++;
				}
			} else {
				*twb1++ = ch;
				col++;
			}
		}
		if (cur == vs->cursor && !leftside)
			ncol = col + pwidth - 1;
		cur++;
	}
	if (cur == vs->cursor)
		ncol = col + pwidth;
	if (col < winwidth) {
		while (col < winwidth) {
			*twb1++ = ' ';
			col++;
		}
	} else
		moreright++;
	*twb1 = ' ';

	col = pwidth;
	cnt = winwidth;
	twb1 = wb1;
	twb2 = wb2;
	while (cnt--) {
		if (*twb1 != *twb2) {
			if (x_col != col)
				ed_mov_opt(col, wb1);
			x_putc(*twb1);
			x_col++;
		}
		twb1++;
		twb2++;
		col++;
	}
	if (vs->winleft > 0 && moreright)
		/*
		 * POSIX says to use * for this but that is a globbing
		 * character and may confuse people; + is more innocuous
		 */
		mc = '+';
	else if (vs->winleft > 0)
		mc = '<';
	else if (moreright)
		mc = '>';
	else
		mc = ' ';
	if (mc != morec) {
		ed_mov_opt(pwidth + winwidth + 1, wb1);
		x_putc(mc);
		x_col++;
		morec = mc;
	}
	if (x_col != ncol)
		ed_mov_opt(ncol, wb1);
}

static void
ed_mov_opt(int col, char *wb)
{
	if (col < x_col) {
		if (col + 1 < x_col - col) {
			x_putc('\r');
			x_pprompt();
			while (x_col++ < col)
				x_putcf(*wb++);
		} else {
			while (x_col-- > col)
				x_putc('\b');
		}
	} else {
		wb = &wb[x_col - pwidth];
		while (x_col++ < col)
			x_putcf(*wb++);
	}
	x_col = col;
}

/* replace word with all expansions (ie, expand word*) */
static int
expand_word(int cmd)
{
	static struct edstate *buf;
	int rval = 0, nwords, start, end, i;
	char **words;

	/* Undo previous expansion */
	if (cmd == 0 && expanded == EXPAND && buf) {
		restore_edstate(vs, buf);
		buf = 0;
		expanded = NONE;
		return (0);
	}
	if (buf) {
		free_edstate(buf);
		buf = 0;
	}

	i = XCF_COMMAND_FILE | XCF_FULLPATH;
	nwords = x_cf_glob(&i, vs->cbuf, vs->linelen, vs->cursor,
	    &start, &end, &words);
	if (nwords == 0) {
		vi_error();
		return (-1);
	}

	buf = save_edstate(vs);
	expanded = EXPAND;
	del_range(start, end);
	vs->cursor = start;
	i = 0;
	while (i < nwords) {
		if (x_escape(words[i], strlen(words[i]), x_vi_putbuf) != 0) {
			rval = -1;
			break;
		}
		if (++i < nwords && putbuf(T1space, 1, Nee) != 0) {
			rval = -1;
			break;
		}
	}
	i = buf->cursor - end;
	if (rval == 0 && i > 0)
		vs->cursor += i;
	modified = 1;
	hnum = hlast;
	insert = INSERT;
	lastac = 0;
	refresh(Nee);
	return (rval);
}

static int
complete_word(int cmd, int count)
{
	static struct edstate *buf;
	int rval, nwords, start, end, flags;
	size_t match_len;
	char **words;
	char *match;
	Wahr is_unique;

	/* Undo previous completion */
	if (cmd == 0 && expanded == COMPLETE && buf) {
		print_expansions(buf, 0);
		expanded = PRINT;
		return (0);
	}
	if (cmd == 0 && expanded == PRINT && buf) {
		restore_edstate(vs, buf);
		buf = 0;
		expanded = NONE;
		return (0);
	}
	if (buf) {
		free_edstate(buf);
		buf = 0;
	}

	/*
	 * XCF_FULLPATH for count 'cause the menu printed by
	 * print_expansions() was done this way.
	 */
	flags = XCF_COMMAND_FILE;
	if (count)
		flags |= XCF_FULLPATH;
	nwords = x_cf_glob(&flags, vs->cbuf, vs->linelen, vs->cursor,
	    &start, &end, &words);
	if (nwords == 0) {
		vi_error();
		return (-1);
	}
	if (count) {
		int i;

		count--;
		if (count >= nwords) {
			vi_error();
			x_print_expansions(nwords, words,
			    isWahr(flags & XCF_IS_COMMAND));
			x_free_words(nwords, words);
			redraw_line(Nee);
			return (-1);
		}
		/*
		 * Expand the count'th word to its basename
		 */
		if (flags & XCF_IS_COMMAND) {
			match = words[count] +
			    x_basename(words[count], NULL);
			/* If more than one possible match, use full path */
			for (i = 0; i < nwords; i++)
				if (i != count &&
				    strcmp(words[i] + x_basename(words[i],
				    NULL), match) == 0) {
					match = words[count];
					break;
				}
		} else
			match = words[count];
		match_len = strlen(match);
		is_unique = Ja;
		/* expanded = PRINT;	next call undo */
	} else {
		match = words[0];
		match_len = x_longest_prefix(nwords, words);
		/* next call will list completions */
		expanded = COMPLETE;
		is_unique = nwords == 1;
	}

	buf = save_edstate(vs);
	del_range(start, end);
	vs->cursor = start;

	/*
	 * escape all shell-sensitive characters and put the result into
	 * command buffer
	 */
	rval = x_escape(match, match_len, x_vi_putbuf);

	if (rval == 0 && is_unique) {
		/*
		 * If exact match, don't undo. Allows directory completions
		 * to be used (ie, complete the next portion of the path).
		 */
		expanded = NONE;

		/*
		 * append a space if this is a non-directory match
		 * and not a parameter substitution, slash for homedir
		 */
		if (match_len > 0 && !mksh_cdirsep(match[match_len - 1])) {
			if (flags & XCF_IS_HOMEDIR)
				rval = putbuf("/", 1, Nee);
			else if (!(flags & XCF_IS_NOSPACE))
				rval = putbuf(T1space, 1, Nee);
		}
	}
	x_free_words(nwords, words);

	modified = 1;
	hnum = hlast;
	insert = INSERT;
	/* prevent this from being redone... */
	lastac = 0;
	refresh(Nee);

	return (rval);
}

static int
print_expansions(struct edstate *est, int cmd MKSH_A_UNUSED)
{
	int start, end, nwords, i;
	char **words;

	i = XCF_COMMAND_FILE | XCF_FULLPATH;
	nwords = x_cf_glob(&i, est->cbuf, est->linelen, est->cursor,
	    &start, &end, &words);
	if (nwords == 0) {
		vi_error();
		return (-1);
	}
	x_print_expansions(nwords, words, isWahr(i & XCF_IS_COMMAND));
	x_free_words(nwords, words);
	redraw_line(Nee);
	return (0);
}

static void
vi_error(void)
{
	/* Beem out of any macros as soon as an error occurs */
	vi_macro_reset();
	x_putc(KSH_BEL);
	x_flush();
}

static void
vi_macro_reset(void)
{
	if (macro.p) {
		afree(macro.buf, AEDIT);
		memset((char *)&macro, 0, sizeof(macro));
	}
}
#endif /* !MKSH_S_NOVI */

/* called from main.c */
void
x_init(void)
{
	size_t i;

	/*
	 * set edchars to force initial binding, except we need
	 * default values for ^W for some deficient systems…
	 */
	edchars.erase = edchars.kill = edchars.intr = edchars.quit =
	    edchars.eof = EDCHAR_INITIAL;
	edchars.werase = 027;

	/* command line editing specific memory allocation */
	ainit(AEDIT);
	holdbufp = alloc(LINE, AEDIT);

	/* initialise Emacs command line editing mode */
	x_nextcmd = -1;

	x_ktab = alloc2(X_NTABS << 8, sizeof(kby), AEDIT);
	i = 0;
	while (i <= 0xFF)
		x_ktab[i++] = XFUNC_insert;
	while (i < (X_NTABS << 8))
		x_ktab[i++] = XFUNC_error;
	for (i = 0; i < NELEM(x_defbindings); ++i)
		X_KTAB(x_defbindings[i].xdb_tab,
		    x_defbindings[i].xdb_char) = x_defbindings[i].xdb_func;

#ifndef MKSH_SMALL
	x_mtab = alloc2(X_NTABS << 8, sizeof(char *), AEDIT);
	i = 0;
	while (i < (X_NTABS << 8))
		x_mtab[i++] = NULL;
#endif

	x_btab = alloc2(X_NTABS << (8 - 3), sizeof(kby), AEDIT);
	memset(x_btab, 0, (X_NTABS << (8 - 3)) * sizeof(kby));
}

#ifdef DEBUG_LEAKS
void
x_done(void)
{
	if (x_btab != NULL)
		afreeall(AEDIT);
}
#endif

void
x_initterm(const char *termtype)
{
	/* default must be 0 (bss) */
	x_term_mode = 0;
	/* catch any of the TERM types tmux uses, don’t ask m̲e̲ about it… */
	switch (*termtype) {
	case 's':
		if (!strncmp(termtype, "screen", 6) &&
		    (!termtype[6] || ord(termtype[6]) == ORD('-')))
			x_term_mode = 1;
		break;
	case 't':
		if (!strncmp(termtype, "tmux", 4) &&
		    (!termtype[4] || ord(termtype[4]) == ORD('-')))
			x_term_mode = 1;
		break;
	}
}

#ifndef MKSH_SMALL
static char *
x_eval_region_helper(const char *cmd, size_t len)
{
	char * volatile cp;
	newenv(E_ERRH);

	if (!kshsetjmp(e->jbuf)) {
		char *wds = alloc(len + 3, ATEMP);

		wds[0] = FUNASUB;
		memcpy(wds + 1, cmd, len);
		wds[len + 1] = '\0';
		wds[len + 2] = EOS;

		cp = evalstr(wds, DOSCALAR);
		afree(wds, ATEMP);
		strdupx(cp, cp, AEDIT);
	} else
		/* command cannot be parsed */
		cp = NULL;
	quitenv(NULL);
	return (cp);
}

static int
x_operate_region(char *(*helper)(const char *, size_t))
{
	char *rgbeg, *rgend, *cp;
	size_t newlen;
	/* only for LINE overflow checking */
	size_t restlen;

	if (xmp == NULL) {
		rgbeg = xbuf;
		rgend = xep;
	} else if (xmp < xcp) {
		rgbeg = xmp;
		rgend = xcp;
	} else {
		rgbeg = xcp;
		rgend = xmp;
	}

	x_e_putb('\r');
	x_clrtoeol(' ', Nee);
	x_flush();
	x_mode(Nee);
	cp = helper(rgbeg, rgend - rgbeg);
	x_mode(Ja);

	if (cp == NULL) {
		/* error return from helper */
 x_eval_region_err:
		x_e_putb(KSH_BEL);
		x_redraw('\r');
		return (KSTD);
	}

	newlen = strlen(cp);
	restlen = xep - rgend;
	/* check for LINE overflow, until this is dynamically allocated */
	if (rgbeg + newlen + restlen >= xend)
		goto x_eval_region_err;

	xmp = rgbeg;
	xcp = rgbeg + newlen;
	xep = xcp + restlen;
	memmove(xcp, rgend, restlen + /* NUL */ 1);
	memcpy(xmp, cp, newlen);
	afree(cp, AEDIT);
	x_adjust();
	x_modified();
	return (KSTD);
}

static int
x_eval_region(int c MKSH_A_UNUSED)
{
	return (x_operate_region(x_eval_region_helper));
}

static char *
x_quote_region_helper(const char *cmd, size_t len)
{
	char *s;
	size_t newlen;
	struct shf shf;

	strndupx(s, cmd, len, ATEMP);
	newlen = len < 256 ? 256 : 4096;
	shf_sreopen(alloc(newlen, AEDIT), newlen, AEDIT, &shf);
	print_value_quoted(&shf, s);
	afree(s, ATEMP);
	return (shf_sclose(&shf));
}

static int
x_quote_region(int c MKSH_A_UNUSED)
{
	return (x_operate_region(x_quote_region_helper));
}
#endif /* !MKSH_SMALL */

#ifdef DF
/* debug code to dump the Emacs buffer state */
#define TL() do {							\
	tmp = ofs & 0xFU;						\
	if (tmp) {							\
		while (tmp != 0x10U) {					\
			shf_puts(tmp == 0x8U ? " -   " : "   ", &to);	\
			++tmp;						\
		}							\
		tmp = ofs & (size_t)~(size_t)0xFU;			\
	} else if (ofs)							\
		tmp = ofs - 0x10U;					\
	shf_puts("  ", &to);						\
	while ((tmp & 0xFU) < lbeg) {					\
		shf_putc(' ', &to);					\
		++tmp;							\
	}								\
	shf_putc('|', &to);						\
	while (tmp < ofs) {						\
		kby c = xbuf[tmp++];					\
		shf_putc(ctype(c, C_PRINT) ? c : '.', &to);		\
	}								\
	shf_putc('|', &to);						\
	lbeg = ofs & 0xFU;						\
	nl = 1;								\
} while (/* CONSTCOND */ 0)

#define DP(p) do { if ((xbuf + ofs) == p) {				\
	if (!nl) TL();							\
	shf_fprintf(&to, nl < 2 ? "\n   ↓ = %s" : " and %s", #p);	\
	nl = 2;								\
} } while (/* CONSTCOND */ 0)

static void
x_emacs_DF(const char *reason)
{
	struct shf to;
	kby nl;
	size_t ofs, end, tmp, lbeg = 0;

	shf_sopen(NULL, 0, SHF_WR | SHF_DYNAMIC, &to);
	ofs = 0;
	nl = 1;
	if (xdp > xep)
		end = xdp - xbuf + 1;
	else
		end = xep - xbuf + 1;
	/* end is within xend + X_EXTRA */
	while (ofs < end) {
		DP(xbp);
		DP(xmp);
		DP(xcp);
		DP(xlp);
		DP(xdp);
		DP(xep);
		if (nl) {
			shf_fprintf(&to, "\n%04zX ", ofs);
			tmp = ofs & (size_t)~(size_t)0xFU;
			while (tmp < ofs) {
				shf_puts((tmp & 0xFU) == 0x8U ?
				    " -   " : "   ", &to);
				++tmp;
			}
			nl = 0;
		}
		shf_fprintf(&to, (ofs & 0xFU) == 0x8U ? " - %02X" : " %02X",
		    ord(xbuf[ofs]));
		++ofs;
		if (!(ofs & 0xFU))
			TL();
	}
	if (!nl)
		TL();
	DF("‣ xbuf: %s%s", reason, shf_sclose(&to));
}

static int
x_debug_buf(int c MKSH_A_UNUSED)
{
	x_emacs_DF("from keyboard");
	return (KSTD);
}
#endif
#endif /* !MKSH_NO_CMDLINE_EDITING */
