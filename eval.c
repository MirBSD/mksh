/*	$OpenBSD: eval.c,v 1.40 2013/09/14 20:09:30 millert Exp $	*/

/*-
 * Copyright (c) 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010,
 *		 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018,
 *		 2019, 2020, 2021, 2022
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

__RCSID("$MirOS: src/bin/mksh/eval.c,v 1.255 2022/07/21 03:57:41 tg Exp $");

/*
 * string expansion
 *
 * first pass: quoting, IFS separation, ~, ${}, $() and $(()) substitution.
 * second pass: alternation ({,}), filename expansion (*?[]).
 */

/* expansion generator state */
typedef struct {
	/* not including an "int type;" member, see expand() */
	/* string */
	const char *str;
	/* source */
	union {
		/* string[] */
		const char **strv;
		/* file */
		struct shf *shf;
	} u;
	/* variable in ${var...} */
	struct tbl *var;
	/* split "$@" / call waitlast in $() */
	Wahr split;
} Expand;

#define XBASE		0	/* scanning original string */
#define XARGSEP		1	/* ifs0 between "$*" */
#define XARG		2	/* expanding $*, $@ */
#define XCOM		3	/* expanding $() */
#define XNULLSUB	4	/* "$@" when $# is 0, so don't generate word */
#define XSUB		5	/* expanding ${} string */
#define XSUBMID		6	/* middle of expanding ${}; must be XSUB+1 */
#define XSUBPAT		7	/* expanding [[ x = ${} ]] string */
#define XSUBPATMID	8	/* middle, must be XSUBPAT+1 */

#define isXSUB(t)	((t) == XSUB || (t) == XSUBPAT)

/* States used for field splitting */
#define IFS_WORD	0	/* word has chars (or quotes except "$@") */
#define IFS_WS		1	/* have seen IFS white-space */
#define IFS_NWS		2	/* have seen IFS non-white-space */
#define IFS_IWS		3	/* beginning of word, ignore IFS WS */
#define IFS_QUOTE	4	/* beg.w/quote, become IFS_WORD unless "$@" */

#define STYPE_CHAR	0xFF
#define STYPE_DBL	0x100
#define STYPE_AT	0x200
#define STYPE_SINGLE	0x2FF
#define STYPE_MASK	0x300

static int varsub(Expand *, const char *, const char *, unsigned int *, int *);
static int comsub(Expand *, const char *, int);
static char *valsub(struct op *, Area *);
static void funsub(struct op *);
static char *trimsub(char *, char *, int);
static void glob(char *, XPtrV *, Wahr);
static void globit(XString *, char **, char *, XPtrV *, int);
static const char *maybe_expand_tilde(const char *, XString *, char **, Wahr);
#ifndef MKSH_NOPWNAM
static char *homedir(char *);
#endif
static void alt_expand(XPtrV *, char *, char *, char *, int);
static int utflen(const char *);
static void utfincptr(const char *, mksh_ari_t *);

/* null (sh.h) */
char null_string[4] = { 0, 0, 0, 0 };

/* UTFMODE functions */
static int
utflen(const char *s)
{
	size_t n;

	if (UTFMODE) {
		n = 0;
		while (*s) {
			s += ez_mbtoc(NULL, s);
			++n;
		}
	} else
		n = strlen(s);

	if (n > 2147483647)
		n = 2147483647;
	return ((int)n);
}

static void
utfincptr(const char *s, mksh_ari_t *lp)
{
	const char *cp = s;

	while ((*lp)--)
		cp += ez_mbtoc(NULL, cp);
	*lp = cp - s;
}

/* compile and expand word */
char *
substitute(const char *cp, int f)
{
	struct source *s, *sold;

	sold = source;
	s = pushs(SWSTR, ATEMP);
	s->start = s->str = cp;
	source = s;
	if (yylex(ONEWORD) != LWORD)
		kerrf(KWF_INTERNAL | KWF_ERR(0xFF) | KWF_ONEMSG | KWF_NOERRNO,
		    Tbadsubst);
	source = sold;
	afree(s, ATEMP);
	return (evalstr(yylval.cp, f));
}

/*
 * expand arg-list
 */
char **
eval(const char **ap, int f)
{
	XPtrV w;

	if (*ap == NULL) {
		union mksh_ccphack vap;

		vap.ro = ap;
		return (vap.rw);
	}
	XPinit(w, 32);
	/* space for shell name */
	XPput(w, NULL);
	while (*ap != NULL)
		expand(*ap++, &w, f);
	XPput(w, NULL);
	return ((char **)XPclose(w) + 1);
}

/*
 * expand string
 */
char *
evalstr(const char *cp, int f)
{
	XPtrV w;
	char *dp = null;

	XPinit(w, 1);
	expand(cp, &w, f);
	if (XPsize(w))
		dp = *XPptrv(w);
	XPfree(w);
	return (dp);
}

/*
 * expand string - return only one component
 * used from iosetup to expand redirection files
 */
char *
evalonestr(const char *cp, int f)
{
	XPtrV w;
	char *rv;

	XPinit(w, 1);
	expand(cp, &w, f);
	switch (XPsize(w)) {
	case 0:
		rv = null;
		break;
	case 1:
		rv = (char *) *XPptrv(w);
		break;
	default:
		rv = evalstr(cp, f & ~DOGLOB);
		break;
	}
	XPfree(w);
	return (rv);
}

/* for nested substitution: ${var:=$var2} */
typedef struct SubType {
	struct tbl *var;	/* variable for ${var..} */
	struct SubType *prev;	/* old type */
	struct SubType *next;	/* poped type (to avoid re-allocating) */
	size_t	base;		/* start position of expanded word */
	unsigned short stype;	/* [=+-?%#] action after expanded word */
	short	f;		/* saved value of f (DOPAT, etc) */
	kby	quotep;		/* saved value of quote (for ${..[%#]..}) */
	kby	quotew;		/* saved value of quote (for ${..[+-=]..}) */
} SubType;

void
expand(
    /* input word */
    const char *ccp,
    /* output words */
    XPtrV *wp,
    /* DO* flags */
    int f)
{
	int c = 0;
	/* expansion type */
	int type;
	/* quoted */
	int quote = 0;
	/* destination string and live pointer */
	XString ds;
	char *dp;
	/* source */
	const char *sp;
	/* second pass flags */
	int fdo;
	/* have word */
	int word;
	/* field splitting of parameter/command substitution */
	int doblank;
	/* expansion variables */
	Expand x = {
		NULL, { NULL }, NULL, 0
	};
	SubType st_head, *st;
	/* record number of trailing newlines in COMSUB */
	int newlines = 0;
	Wahr saw_eq, make_magic;
	unsigned int tilde_ok;
	size_t len;
	char *cp;

	if (ccp == NULL)
		kerrf(KWF_INTERNAL | KWF_ERR(0xFF) | KWF_ONEMSG | KWF_NOERRNO,
		    "expand(NULL)");
	/* for alias, readonly, set, typeset commands */
	if ((f & DOVACHECK) && is_wdvarassign(ccp)) {
		f &= ~(DOVACHECK | DOBLANK | DOGLOB | DOTILDE);
		f |= DOASNTILDE | DOSCALAR;
	}
	if (Flag(FNOGLOB))
		f &= ~DOGLOB;
	if (Flag(FMARKDIRS))
		f |= DOMARKDIRS;
	if (Flag(FBRACEEXPAND) && (f & DOGLOB))
		f |= DOBRACE;

	/* init destination string */
	Xinit(ds, dp, 128, ATEMP);
	type = XBASE;
	sp = ccp;
	fdo = 0;
	saw_eq = Nee;
	/* must be 1/0 */
	tilde_ok = (f & (DOTILDE | DOASNTILDE)) ? 1 : 0;
	doblank = 0;
	make_magic = Nee;
	word = (f&DOBLANK) ? IFS_WS : IFS_WORD;
	/* clang doesn't know OSUBST comes before CSUBST */
	memset(&st_head, 0, sizeof(st_head));
	st = &st_head;

	while (/* CONSTCOND */ 1) {
		Xcheck(ds, dp);

		switch (type) {
		case XBASE:
			/* original prefixed string */
			c = ord(*sp++);
			switch (c) {
			case EOS:
				c = 0;
				break;
			case CHAR:
				c = ord(*sp++);
				break;
			case QCHAR:
				/* temporary quote */
				quote |= 2;
				c = ord(*sp++);
				break;
			case OQUOTE:
				if (word != IFS_WORD)
					word = IFS_QUOTE;
				tilde_ok = 0;
				quote = 1;
				continue;
			case CQUOTE:
				if (word == IFS_QUOTE)
					word = IFS_WORD;
				quote = st->quotew;
				continue;
			case COMASUB:
			case COMSUB:
			case FUNASUB:
			case FUNSUB:
			case VALSUB:
				tilde_ok = 0;
				if (f & DONTRUNCOMMAND) {
					word = IFS_WORD;
					*dp++ = '$';
					switch (c) {
					case COMASUB:
					case COMSUB:
						*dp++ = '(';
						c = ORD(')');
						break;
					case FUNASUB:
					case FUNSUB:
					case VALSUB:
						*dp++ = '{';
						*dp++ = c == VALSUB ? '|' : ' ';
						c = ORD('}');
						break;
					}
					while (*sp != '\0') {
						Xcheck(ds, dp);
						*dp++ = *sp++;
					}
					if ((unsigned int)c == ORD(/*{*/'}'))
						*dp++ = ';';
					*dp++ = c;
				} else {
					type = comsub(&x, sp, c);
					if (type != XBASE && (f & DOBLANK))
						doblank++;
					sp = strnul(sp) + 1;
					newlines = 0;
				}
				continue;
			case EXPRSUB:
				tilde_ok = 0;
				if (f & DONTRUNCOMMAND) {
					word = IFS_WORD;
					*dp++ = '$'; *dp++ = '('; *dp++ = '(';
					while (*sp != '\0') {
						Xcheck(ds, dp);
						*dp++ = *sp++;
					}
					*dp++ = ')'; *dp++ = ')';
				} else {
					struct tbl v;

					v.flag = DEFINED|ISSET|INTEGER;
					/* not default */
					v.type = 10;
					v.name[0] = '\0';
					v_evaluate(&v, substitute(sp, 0),
					    KSH_UNWIND_ERROR, Ja);
					sp = strnul(sp) + 1;
					x.str = str_val(&v);
					type = XSUB;
					if (f & DOBLANK)
						doblank++;
				}
				continue;
			case OSUBST: {
				/* ${{#}var{:}[=+-?#%]word} */
			/*-
			 * format is:
			 *	OSUBST [{x] plain-variable-part \0
			 *	    compiled-word-part CSUBST [}x]
			 * This is where all syntax checking gets done...
			 */
				/* skip the { or x (}) */
				const char *varname = ++sp;
				unsigned int stype /* for GCC */ = 0;
				int slen = 0;

				/* skip variable */
				sp = cstrchr(sp, '\0') + 1;
				type = varsub(&x, varname, sp, &stype, &slen);
				if (type < 0) {
					char *beg, *end, *str;
 unwind_substsyn:
					/* restore sp */
					sp = varname - 2;
					beg = wdcopy(sp, ATEMP);
					end = (wdscan(cstrchr(sp, '\0') + 1,
					    CSUBST) - sp) + beg;
					/* ({) the } or x is already skipped */
					if (end < wdscan(beg, EOS))
						*end = EOS;
					str = snptreef(NULL, 64, Tf_S, beg);
					afree(beg, ATEMP);
					kerrf(KWF_ERR(1) | KWF_PREFIX |
					    KWF_FILELINE | KWF_TWOMSG |
					    KWF_NOERRNO, str, Tbadsubst);
				}
				if (f & DOBLANK)
					doblank++;
				tilde_ok = 0;
				if (word == IFS_QUOTE && type != XNULLSUB)
					word = IFS_WORD;
				if (type == XBASE) {
					/* expand? */
					if (!st->next) {
						SubType *newst;

						newst = alloc(sizeof(SubType), ATEMP);
						newst->next = NULL;
						newst->prev = st;
						st->next = newst;
					}
					st = st->next;
					st->stype = stype;
					st->base = Xsavepos(ds, dp);
					st->f = f;
					if (x.var == vtemp) {
						st->var = tempvar(vtemp->name);
						st->var->flag &= ~INTEGER;
						/* can't fail here */
						setstr(st->var,
						    str_val(x.var),
						    KSH_RETURN_ERROR | 0x4);
					} else
						st->var = x.var;

					st->quotew = st->quotep = quote;
					/* skip qualifier(s) */
					if (stype)
						sp += slen;
					switch (stype & STYPE_SINGLE) {
					case ORD('#') | STYPE_AT:
					case ORD('Q') | STYPE_AT:
					case ORD('^') | STYPE_AT:
						break;
					case ORD('0'): {
						char *beg, *mid, *end, *stg;
						mksh_ari_t from = 0, num = -1, flen, finc = 0;

						beg = wdcopy(sp, ATEMP);
						mid = beg + (wdscan(sp, ADELIM) - sp);
						stg = beg + (wdscan(sp, CSUBST) - sp);
						mid[-2] = EOS;
						if (ord(mid[-1]) == ORD(/*{*/ '}')) {
							sp += mid - beg - 1;
							end = NULL;
						} else {
							end = mid +
							    (wdscan(mid, ADELIM) - mid);
							if (ord(end[-1]) != ORD(/*{*/ '}'))
								/* more than max delimiters */
								goto unwind_substsyn;
							end[-2] = EOS;
							sp += end - beg - 1;
						}
						evaluate(substitute(stg = wdstrip(beg, 0), 0),
						    &from, KSH_UNWIND_ERROR, Ja);
						afree(stg, ATEMP);
						if (end) {
							evaluate(substitute(stg = wdstrip(mid, 0), 0),
							    &num, KSH_UNWIND_ERROR, Ja);
							afree(stg, ATEMP);
						}
						afree(beg, ATEMP);
						beg = str_val(st->var);
						flen = utflen(beg);
						if (from < 0) {
							if (-from < flen)
								finc = flen + from;
						} else
							finc = from < flen ? from : flen;
						if (UTFMODE)
							utfincptr(beg, &finc);
						beg += finc;
						flen = utflen(beg);
						if (num < 0 || num > flen)
							num = flen;
						if (UTFMODE)
							utfincptr(beg, &num);
						strndupx(x.str, beg, num, ATEMP);
						goto do_CSUBST;
					    }
					case ORD('/') | STYPE_AT:
					case ORD('/'): {
						char *s, *p, *d, *sbeg;
						char *pat = NULL, *rrep;
						char fpat = 0, *tpat1, *tpat2;
						char *ws, *wpat, *wrep, tch;
						size_t rreplen;

						s = ws = wdcopy(sp, ATEMP);
						p = s + (wdscan(sp, ADELIM) - sp);
						d = s + (wdscan(sp, CSUBST) - sp);
						p[-2] = EOS;
						if (ord(p[-1]) == ORD(/*{*/ '}'))
							d = NULL;
						else
							d[-2] = EOS;
						sp += (d ? d : p) - s - 1;
						if (!(stype & STYPE_MASK) &&
						    s[0] == CHAR &&
						    ctype(s[1], C_SUB2))
							fpat = s[1];
						wpat = s + (fpat ? 2 : 0);
						if (!(wrep = d ? p : NULL)) {
							rrep = null;
							rreplen = 0;
						} else if (!(stype & STYPE_AT)) {
							rrep = evalstr(wrep,
							    DOTILDE | DOSCALAR);
							rreplen = strlen(rrep);
						} else {
							rrep = NULL;
							/* shut up GCC */
							rreplen = 0;
						}

						/* prepare string on which to work */
						strdupx(s, str_val(st->var), ATEMP);
						sbeg = s;
 again_search:
						pat = evalstr(wpat,
						    DOTILDE | DOSCALAR | DOPAT);
						/* check for special cases */
						if (!*pat && !fpat) {
							/*
							 * empty unanchored
							 * pattern => reject
							 */
							goto no_repl;
						}
						if ((stype & STYPE_MASK) &&
						    gmatchx(null, pat, Nee)) {
							/*
							 * pattern matches empty
							 * string => don't loop
							 */
							stype &= ~STYPE_MASK;
						}

						/* first see if we have any match at all */
						if (ord(fpat) == ORD('#')) {
							/* anchor at the beginning */
							tpat1 = shf_smprintf("%s%c*", pat, MAGIC);
							tpat2 = tpat1;
						} else if (ord(fpat) == ORD('%')) {
							/* anchor at the end */
							tpat1 = shf_smprintf("%c*%s", MAGIC, pat);
							tpat2 = pat;
						} else {
							/* float */
							tpat1 = shf_smprintf("%c*%s%c*", MAGIC, pat, MAGIC);
							tpat2 = tpat1 + 2;
						}
 again_repl:
						/*
						 * this would not be necessary if gmatchx would return
						 * the start and end values of a match found, like re*
						 */
						if (!gmatchx(sbeg, tpat1, Nee))
							goto end_repl;
						d = strnul(s);
						/* now anchor the beginning of the match */
						if (ord(fpat) != ORD('#'))
							while (sbeg <= d) {
								if (gmatchx(sbeg, tpat2, Nee))
									break;
								else
									sbeg++;
							}
						/* now anchor the end of the match */
						p = d;
						if (ord(fpat) != ORD('%'))
							while (p >= sbeg) {
								Wahr gotmatch;

								c = ord(*p);
								*p = '\0';
								gotmatch = isWahr(gmatchx(sbeg, pat, Nee));
								*p = c;
								if (gotmatch)
									break;
								p--;
							}

						/* record partial string as match */
						tch = *p;
						*p = '\0';
						record_match(sbeg);
						*p = tch;
						/* get replacement string, if necessary */
						if ((stype & STYPE_AT) &&
						    rrep != null) {
							afree(rrep, ATEMP);
							/* might access match! */
							rrep = evalstr(wrep,
							    DOTILDE | DOSCALAR);
							rreplen = strlen(rrep);
						}

						/*
						 * string:
						 * |--------|---------|-------\0
						 * s  n1    sbeg  n2  p  n3   d
						 *
						 * replacement:
						 *          |------------|
						 *          rrep  rreplen
						 */

						/* move strings around and replace */
						{
							size_t n1 = sbeg - s;
							size_t n2 = p - sbeg;
							size_t n3 = d - p;
							/* move part3 to the front, OR… */
							if (rreplen < n2)
								memmove(sbeg + rreplen,
								    p, n3 + 1);
							/* … adjust size, move to back */
							if (rreplen > n2) {
								s = aresize(s,
								    n1 + rreplen + n3 + 1,
								    ATEMP);
								memmove(s + n1 + rreplen,
								    s + n1 + n2,
								    n3 + 1);
							}
							/* insert replacement */
							if (rreplen)
								memcpy(s + n1, rrep, rreplen);
							/* continue after the place */
							sbeg = s + n1 + rreplen;
						}
						if (stype & STYPE_AT) {
							afree(tpat1, ATEMP);
							afree(pat, ATEMP);
							goto again_search;
						} else if (stype & STYPE_DBL)
							goto again_repl;
 end_repl:
						afree(tpat1, ATEMP);
						x.str = s;
 no_repl:
						afree(pat, ATEMP);
						if (rrep != null)
							afree(rrep, ATEMP);
						afree(ws, ATEMP);
						goto do_CSUBST;
					    }
					case ORD('#'):
					case ORD('%'):
						/* ! DOBLANK,DOBRACE */
						f = (f & DONTRUNCOMMAND) |
						    DOPAT | DOTILDE |
						    DOTEMP | DOSCALAR;
						tilde_ok = 1;
						st->quotew = quote = 0;
						/*
						 * Prepend open pattern (so |
						 * in a trim will work as
						 * expected)
						 */
						if (!Flag(FSH)) {
							*dp++ = MAGIC;
							*dp++ = ORD(0x80 | '@');
						}
						break;
					case ORD('='):
						/*
						 * Tilde expansion for string
						 * variables in POSIX mode is
						 * governed by Austinbug 351.
						 * In non-POSIX mode historic
						 * ksh behaviour (enable it!)
						 * us followed.
						 * Not doing tilde expansion
						 * for integer variables is a
						 * non-POSIX thing - makes
						 * sense though, since ~ is
						 * a arithmetic operator.
						 */
						if (!(x.var->flag & INTEGER))
							f |= DOASNTILDE | DOTILDE;
						f |= DOTEMP | DOSCALAR;
						/*
						 * These will be done after the
						 * value has been assigned.
						 */
						f &= ~(DOBLANK|DOGLOB|DOBRACE);
						tilde_ok = 1;
						break;
					case ORD('?'):
						if (*sp == CSUBST)
							kerrf(KWF_ERR(1) | KWF_PREFIX |
							    KWF_FILELINE | KWF_TWOMSG |
							    KWF_NOERRNO, st->var->name,
							    "parameter null or not set");
						f &= ~DOBLANK;
						f |= DOTEMP;
						/* FALLTHROUGH */
					default:
						/* '-' '+' '?' */
						if (quote)
							word = IFS_WORD;
						else if (dp == Xstring(ds, dp))
							word = IFS_IWS;
						/* Enable tilde expansion */
						tilde_ok = 1;
						f |= DOTILDE;
					}
				} else
					/* skip word */
					sp += wdscan(sp, CSUBST) - sp;
				continue;
			    }
			case CSUBST:
				/* only get here if expanding word */
 do_CSUBST:
				/* ({) skip the } or x */
				sp++;
				/* in case of ${unset:-} */
				tilde_ok = 0;
				*dp = '\0';
				quote = st->quotep;
				f = st->f;
				if (f & DOBLANK)
					doblank--;
				switch (st->stype & STYPE_SINGLE) {
				case ORD('#'):
				case ORD('%'):
					if (!Flag(FSH)) {
						/* Append end-pattern */
						*dp++ = MAGIC;
						*dp++ = ')';
					}
					*dp = '\0';
					dp = Xrestpos(ds, dp, st->base);
					/*
					 * Must use st->var since calling
					 * global would break things
					 * like x[i+=1].
					 */
					x.str = trimsub(str_val(st->var),
						dp, st->stype);
					if (x.str[0] != '\0') {
						word = IFS_IWS;
						type = XSUB;
					} else if (quote) {
						word = IFS_WORD;
						type = XSUB;
					} else {
						if (dp == Xstring(ds, dp))
							word = IFS_IWS;
						type = XNULLSUB;
					}
					if (f & DOBLANK)
						doblank++;
					st = st->prev;
					continue;
				case ORD('='):
					/*
					 * Restore our position and substitute
					 * the value of st->var (may not be
					 * the assigned value in the presence
					 * of integer/right-adj/etc attributes).
					 */
					dp = Xrestpos(ds, dp, st->base);
					/*
					 * Must use st->var since calling
					 * global would cause with things
					 * like x[i+=1] to be evaluated twice.
					 */
					/*
					 * Note: not exported by FEXPORT
					 * in AT&T ksh.
					 */
					/*
					 * XXX POSIX says read-only is only
					 * fatal for special builtins (setstr
					 * does read-only check).
					 */
					len = strlen(dp) + 1;
					setstr(st->var,
					    debunk(alloc(len, ATEMP),
					    dp, len), KSH_UNWIND_ERROR);
					x.str = str_val(st->var);
					type = XSUB;
					if (f & DOBLANK)
						doblank++;
					st = st->prev;
					word = quote || (!*x.str && (f & DOSCALAR)) ? IFS_WORD : IFS_IWS;
					continue;
				case ORD('?'):
					dp = Xrestpos(ds, dp, st->base);

					kerrf(KWF_ERR(1) | KWF_PREFIX |
					    KWF_FILELINE | KWF_TWOMSG |
					    KWF_NOERRNO, st->var->name,
					    debunk(dp, dp, strlen(dp) + 1));
					break;
				case ORD('#') | STYPE_AT:
					x.str = shf_smprintf(Thex32,
					    (unsigned int)hash(str_val(st->var)));
					goto common_CSUBST;
				case ORD('Q') | STYPE_AT: {
					struct shf shf;

					shf_sopen(NULL, 0, SHF_WR|SHF_DYNAMIC, &shf);
					print_value_quoted(&shf, str_val(st->var));
					x.str = shf_sclose(&shf);
					goto common_CSUBST;
				    }
				case ORD('^') | STYPE_AT: {
					struct shf shf;

					shf_sopen(NULL, 0, SHF_WR|SHF_DYNAMIC, &shf);
					uprntmbs(str_val(st->var), Ja, &shf);
					x.str = shf_sclose(&shf);
					goto common_CSUBST;
				    }
				case ORD('0'):
				case ORD('/') | STYPE_AT:
				case ORD('/'):
 common_CSUBST:
					dp = Xrestpos(ds, dp, st->base);
					type = XSUB;
					word = quote || (!*x.str && (f & DOSCALAR)) ? IFS_WORD : IFS_IWS;
					if (f & DOBLANK)
						doblank++;
					st = st->prev;
					continue;
				/* default: '-' '+' */
				}
				st = st->prev;
				type = XBASE;
				continue;

			case OPAT:
				/* open pattern: *(foo|bar) */
				/* Next char is the type of pattern */
				make_magic = Ja;
				c = ord(*sp++) | 0x80U;
				break;

			case SPAT:
				/* pattern separator (|) */
				make_magic = Ja;
				c = ORD('|');
				break;

			case CPAT:
				/* close pattern */
				make_magic = Ja;
				c = ORD(/*(*/ ')');
				break;
			}
			break;

		case XNULLSUB:
			/*
			 * Special case for "$@" (and "${foo[@]}") - no
			 * word is generated if $# is 0 (unless there is
			 * other stuff inside the quotes).
			 */
			type = XBASE;
			if (f & DOBLANK) {
				doblank--;
				if (dp == Xstring(ds, dp) && word != IFS_WORD)
					word = IFS_IWS;
			}
			continue;

		case XSUBPAT:
		case XSUBPATMID:
 XSUBPAT_beg:
			switch ((c = ord(*x.str++))) {
			case 0:
				goto XSUB_end;
			case ORD('\\'):
				if ((c = ord(*x.str)) == 0)
					/* keep backslash at EOS */
					c = ORD('\\');
				else
					++x.str;
				quote |= 2;
				break;
			/* ctype(c, C_PATMO) */
			case ORD('!'):
			case ORD('*'):
			case ORD('+'):
			case ORD('?'):
			case ORD('@'):
				if (ord(*x.str) == ORD('('/*)*/)) {
					++x.str;
					c |= 0x80U;
					make_magic = Ja;
				}
				break;
			case ORD('('):
				c = ORD(' ') | 0x80U;
				/* FALLTHROUGH */
			case ORD('|'):
			case ORD(')'):
				make_magic = Ja;
				break;
			}
			break;

		case XSUB:
			if (!quote && (f & DODBMAGIC)) {
				const char *cs = x.str;
				int level = 0;

				while ((c = *cs++))
					switch (c) {
					case '\\':
						if ((c = *cs))
							++cs;
						break;
					case ORD('('):
						++level;
						break;
					case ORD(')'):
						--level;
						break;
					}
				/* balanced parentheses? */
				if (!level) {
					type = XSUBPAT;
					goto XSUBPAT_beg;
				}
			}
			/* FALLTHROUGH */
		case XSUBMID:
			if ((c = ord(*x.str++)) == 0) {
 XSUB_end:
				type = XBASE;
				if (f & DOBLANK)
					doblank--;
				continue;
			}
			break;

		case XARGSEP:
			type = XARG;
			quote = 1;
			/* FALLTHROUGH */
		case XARG:
			if ((c = ord(*x.str++)) == '\0') {
				/*
				 * force null words to be created so
				 * set -- "" 2 ""; echo "$@" will do
				 * the right thing
				 */
				if (quote && x.split)
					word = IFS_WORD;
				if ((x.str = *x.u.strv++) == NULL) {
					type = XBASE;
					if (f & DOBLANK)
						doblank--;
					continue;
				}
				c = ord(ifs0);
				if ((f & DOHEREDOC)) {
					/* pseudo-field-split reliably */
					if (c == 0)
						c = ORD(' ');
					break;
				}
				if ((f & DOSCALAR)) {
					/* do not field-split */
					if (x.split) {
						c = ORD(' ');
						break;
					}
					if (c == 0)
						continue;
				}
				if (c == 0) {
					if (quote && !x.split)
						continue;
					if (!quote && word == IFS_WS)
						continue;
					/* this is so we don't terminate */
					c = ORD(' ');
					/* now force-emit a word */
					goto emit_word;
				}
				if (quote && x.split) {
					/* terminate word for "$@" */
					type = XARGSEP;
					quote = 0;
				}
			}
			break;

		case XCOM:
			if (x.u.shf == NULL) {
				/* $(<...) failed */
				subst_exstat = 1;
				/* fake EOF */
				c = -1;
			} else if (newlines) {
				/* spit out saved NLs */
				c = ORD('\n');
				--newlines;
			} else {
				while (c = shf_getc(x.u.shf),
				    cinttype(c, C_NL | C_NUL)) {
#ifdef MKSH_WITH_TEXTMODE
					if (c == ORD('\r')) {
						c = shf_getc(x.u.shf);
						switch (c) {
						case ORD('\n'):
							break;
						default:
							shf_ungetc(c, x.u.shf);
							/* FALLTHROUGH */
						case -1:
							c = ORD('\r');
							break;
						}
					}
#endif
					if (c == ORD('\n'))
						/* save newlines */
						newlines++;
				}
				if (newlines && c != -1) {
					shf_ungetc(c, x.u.shf);
					c = ORD('\n');
					--newlines;
				}
			}
			if (c == -1) {
				newlines = 0;
				if (x.u.shf)
					shf_close(x.u.shf);
				if (x.split)
					subst_exstat = waitlast();
				type = XBASE;
				if (f & DOBLANK)
					doblank--;
				continue;
			}
			break;
		}

		/* check for end of word or IFS separation */
		if (c == 0 || (!quote && (f & DOBLANK) && doblank &&
		    !make_magic && ctype(c, C_IFS))) {
			/*-
			 * How words are broken up:
			 *			|	value of c
			 *	word		|	ws	nws	0
			 *	-----------------------------------
			 *	IFS_WORD		w/WS	w/NWS	w
			 *	IFS_WS			-/WS	-/NWS	-
			 *	IFS_NWS			-/NWS	w/NWS	-
			 *	IFS_IWS			-/WS	w/NWS	-
			 * (w means generate a word)
			 */
			if ((word == IFS_WORD) || (word == IFS_QUOTE) || (c &&
			    (word == IFS_IWS || word == IFS_NWS) &&
			    !ctype(c, C_IFSWS))) {
				size_t dlen;
 emit_word:
				if (f & DOHERESTR)
					*dp++ = '\n';
				*dp++ = '\0';
				dlen = Xlength(ds, dp);
				cp = Xclose(ds, dp);
				if (fdo & DOBRACE)
					/* also does globbing */
					alt_expand(wp, cp, cp,
					    cp + dlen - 1,
					    fdo | (f & DOMARKDIRS));
				else if (fdo & DOGLOB)
					glob(cp, wp, isWahr(f & DOMARKDIRS));
				else if ((f & DOPAT) || !(fdo & DOMAGIC))
					XPput(*wp, cp);
				else
					XPput(*wp, debunk(cp, cp, dlen));
				fdo = 0;
				saw_eq = Nee;
				/* must be 1/0 */
				tilde_ok = (f & (DOTILDE | DOASNTILDE)) ? 1 : 0;
				if (c == 0)
					return;
				Xinit(ds, dp, 128, ATEMP);
			} else if (c == 0) {
				return;
			} else if (isXSUB(type) && ctype(c, C_IFS) &&
			    !ctype(c, C_IFSWS) && Xlength(ds, dp) == 0) {
				*(cp = alloc(1, ATEMP)) = '\0';
				XPput(*wp, cp);
				++type;
			}
			if (word != IFS_NWS)
				word = ctype(c, C_IFSWS) ? IFS_WS : IFS_NWS;
		} else {
			if (isXSUB(type))
				++type;

			/* age tilde_ok info - ~ code tests second bit */
			tilde_ok <<= 1;
			/* mark any special second pass chars */
			if (!quote)
				switch (ord(c)) {
				case ORD('['):
				case ORD('!'):
				case ORD('-'):
				case ORD(']'):
					/*
					 * For character classes - doesn't hurt
					 * to have magic !,-,]s outside of
					 * [...] expressions.
					 */
					if (f & (DOPAT | DOGLOB)) {
						fdo |= DOMAGIC;
						if ((unsigned int)c == ORD('['))
							fdo |= f & DOGLOB;
						*dp++ = MAGIC;
					}
					break;
				case ORD('*'):
				case ORD('?'):
					if (f & (DOPAT | DOGLOB)) {
						fdo |= DOMAGIC | (f & DOGLOB);
						*dp++ = MAGIC;
					}
					break;
				case ORD('{'):
				case ORD('}'):
				case ORD(','):
					if ((f & DOBRACE) &&
					    (ord(c) == ORD('{' /*}*/) ||
					    (fdo & DOBRACE))) {
						fdo |= DOBRACE|DOMAGIC;
						*dp++ = MAGIC;
					}
					break;
				case ORD('='):
					/* Note first unquoted = for ~ */
					if (!(f & DOTEMP) && (!Flag(FPOSIX) ||
					    (f & DOASNTILDE)) && !saw_eq) {
						saw_eq = Ja;
						tilde_ok = 1;
					}
					break;
				case ORD(':'):
					/* : */
					/* Note unquoted : for ~ */
					if (!(f & DOTEMP) && (f & DOASNTILDE))
						tilde_ok = 1;
					break;
				case ORD('~'):
					/*
					 * tilde_ok is reset whenever
					 * any of ' " $( $(( ${ } are seen.
					 * Note that tilde_ok must be preserved
					 * through the sequence ${A=a=}~
					 */
					if (type == XBASE &&
					    (f & (DOTILDE | DOASNTILDE)) &&
					    (tilde_ok & 2)) {
						const char *tcp;
						char *tdp = dp;

						tcp = maybe_expand_tilde(sp,
						    &ds, &tdp,
						    isWahr(f & DOASNTILDE));
						if (tcp) {
							if (dp != tdp)
								word = IFS_WORD;
							dp = tdp;
							sp = tcp;
							continue;
						}
					}
					break;
				}
			else
				/* undo temporary */
				quote &= ~2;

			if (make_magic) {
				make_magic = Nee;
				fdo |= DOMAGIC | (f & DOGLOB);
				*dp++ = MAGIC;
			} else if (ISMAGIC(c)) {
				fdo |= DOMAGIC;
				*dp++ = MAGIC;
			}
			/* save output char */
			*dp++ = c;
			word = IFS_WORD;
		}
	}
}

static Wahr
hasnonempty(const char **strv)
{
	size_t i = 0;

	while (strv[i])
		if (*strv[i++])
			return (Ja);
	return (Nee);
}

/*
 * Prepare to generate the string returned by ${} substitution.
 */
static int
varsub(Expand *xp, const char *sp, const char *word,
    /* becomes qualifier type */
    unsigned int *stypep,
    /* becomes qualifier type len (=, :=, etc.) valid iff *stypep != 0 */
    int *slenp)
{
	unsigned int c;
	int state;		/* next state: XBASE, XARG, XSUB, XNULLSUB */
	unsigned int stype;	/* substitution type */
	int slen = 0;
	const char *p;
	struct tbl *vp;
	Wahr zero_ok = Nee;
	int sc;
	XPtrV wv;

	if ((stype = ord(sp[0])) == '\0')
		/* Bad variable name */
		return (-1);

	xp->var = NULL;

	/* entirety of named array? */
	if ((p = cstrchr(sp, '[')) && (sc = ord(p[1])) &&
	    ord(p[2]) == ORD(']'))
		/* keep p (for ${!foo[1]} below)! */
		switch (sc) {
		case ORD('*'):
			sc = 3;
			break;
		case ORD('@'):
			sc = 7;
			break;
		default:
			/* bit2 = @, bit1 = array, bit0 = enabled */
			sc = 0;
		}
	else
		/* $* and $@ checked below */
		sc = 0;

	/*-
	 * ${%var}, string width (-U: screen columns, +U: octets)
	 * ${#var}, string length (-U: characters, +U: octets) or array size
	 * ${!var}, variable name
	 * ${*…} -> set flag for argv
	 * ${@…} -> set flag for argv
	 */
	if (ctype(stype, C_SUB2 | CiVAR1)) {
		switch (stype) {
		case ORD('*'):
			if (!sc)
				sc = 1;
			goto nopfx;
		case ORD('@'):
			if (!sc)
				sc = 5;
			goto nopfx;
		}
		/* varname required */
		if ((c = ord(sp[1])) == '\0') {
			if (stype == ORD('%'))
				/* $% */
				return (-1);
			/* $# or $! */
			goto nopfx;
		}
		/* can’t have any modifiers for ${#…} or ${%…} or ${!…} */
		if (*word != CSUBST)
			return (-1);
		/* check for argv past prefix */
		if (!sc) switch (c) {
		case ORD('*'):
			sc = 1;
			break;
		case ORD('@'):
			sc = 5;
			break;
		}
		/* skip past prefix */
		++sp;
		/* determine result */
		switch (stype) {
		case ORD('!'):
			if (sc & 2) {
				stype = 0;
				XPinit(wv, 32);
				vp = arraybase(sp);
				do {
					if (vp->flag & ISSET)
						XPput(wv, shf_smprintf(Tf_lu,
						    arrayindex(vp)));
				} while ((vp = vp->u.array));
				goto arraynames;
			}
			xp->var = global(sp);
			/* use saved p from above */
			xp->str = p ? shf_smprintf(Tf_sSQlu, xp->var->name,
			    arrayindex(xp->var)) : xp->var->name;
			break;
#ifdef DEBUG
		default:
			kerrf(KWF_INTERNAL | KWF_ERR(0xFF) | KWF_ONEMSG | KWF_NOERRNO,
			    "stype mismatch");
			/* NOTREACHED */
#endif
		case ORD('%'):
			/* cannot do this on an array */
			if (sc)
				return (-1);
			p = str_val(global(sp));
			zero_ok = p != null;
			/* partial utf_mbswidth reimplementation */
			sc = 0;
			while (*p) {
				p += ez_mbtowc(&c, p);
				/* c == char or wchar at p++ */
				if ((slen = utf_wcwidth(c)) == -1) {
					/* 646, 8859-1, 10646 C0/C1 */
					sc = -1;
					break;
				}
				sc += slen;
			}
			if (0)
				/* FALLTHROUGH */
		case ORD('#'):
			  switch (sc & 3) {
			case 3:
				vp = arraybase(sp);
				if (vp->flag & (ISSET|ARRAY))
					zero_ok = Ja;
				sc = 0;
				do {
					if (vp->flag & ISSET)
						sc++;
				} while ((vp = vp->u.array));
				break;
			case 1:
				sc = e->loc->argc;
				break;
			default:
				p = str_val(global(sp));
				zero_ok = p != null;
				sc = utflen(p);
				break;
			}
			/* ${%var} also here */
			if (Flag(FNOUNSET) && sc == 0 && !zero_ok)
				kerrf(KWF_ERR(1) | KWF_PREFIX |
				    KWF_FILELINE | KWF_TWOMSG |
				    KWF_NOERRNO, sp, Tf_parm);
			xp->str = shf_smprintf(Tf_d, sc);
			break;
		}
		/* unqualified variable/string substitution */
		*stypep = 0;
		return (XSUB);
	}
 nopfx:

	/* check for qualifiers in word part */
	stype = 0;
	/*slen = 0;*/
	c = word[/*slen +*/ 0] == CHAR ? ord(word[/*slen +*/ 1]) : 0;
	if (c == ORD(':')) {
		slen += 2;
		stype = STYPE_DBL;
		c = word[slen + 0] == CHAR ? ord(word[slen + 1]) : 0;
	}
	if (!stype && c == ORD('/')) {
		slen += 2;
		stype = c;
		if (word[slen] == ADELIM &&
		    ord(word[slen + 1]) == c) {
			slen += 2;
			stype |= STYPE_DBL;
		}
	} else if (stype == STYPE_DBL && (c == ORD(' ') || c == ORD('0'))) {
		stype |= ORD('0');
	} else if (ctype(c, C_SUB1)) {
		slen += 2;
		stype |= c;
	} else if (ctype(c, C_SUB2)) {
		/* Note: ksh88 allows :%, :%%, etc */
		slen += 2;
		stype = c;
		if (word[slen + 0] == CHAR && ord(word[slen + 1]) == c) {
			stype |= STYPE_DBL;
			slen += 2;
		}
	} else if (c == ORD('@')) {
		/* @x where x is command char */
		switch (c = ord(word[slen + 2]) == CHAR ?
		    ord(word[slen + 3]) : 0) {
		case ORD('#'):
		case ORD('/'):
		case ORD('Q'):
		case ORD('^'):
			break;
		default:
			return (-1);
		}
		stype |= STYPE_AT | c;
		slen += 4;
	} else if (stype)
		/* : is not ok */
		return (-1);
	if (!stype && *word != CSUBST)
		return (-1);

	if (!sc) {
		xp->var = global(sp);
		xp->str = str_val(xp->var);
		/* can't assign things like $! or $1 */
		if ((stype & STYPE_SINGLE) == ORD('=') &&
		    !*xp->str && ctype(*sp, C_VAR1 | C_DIGIT))
			return (-1);
		state = XSUB;
	} else {
		/* can’t assign/trim a vector (yet) */
		switch (stype & STYPE_SINGLE) {
		case ORD('-'):
		case ORD('+'):
			/* allowed ops */
		case 0:
			/* or no ops */
			break;
	/*	case ORD('='):
		case ORD('?'):
		case ORD('#'):
		case ORD('%'):
		case ORD('/'):
		case ORD('/') | STYPE_AT:
		case ORD('0'):
		case ORD('#') | STYPE_AT:
		case ORD('Q') | STYPE_AT:
		case ORD('^') | STYPE_AT:
	*/	default:
			return (-1);
		}
		/* do what we can */
		if (sc & 2) {
			XPinit(wv, 32);
			vp = arraybase(sp);
			do {
				if (vp->flag & ISSET)
					XPput(wv, str_val(vp));
			} while ((vp = vp->u.array));
 arraynames:
			if ((c = (XPsize(wv) == 0)))
				XPfree(wv);
			else {
				XPput(wv, NULL);
				xp->u.strv = (const char **)XPptrv(wv);
			}
		} else {
			if ((c = (e->loc->argc == 0)))
				xp->var = global(sp);
			else
				xp->u.strv = (const char **)e->loc->argv + 1;
			/* POSIX 2009? */
			zero_ok = Ja;
		}
		/* have we got any elements? */
		if (c) {
			/* no */
			xp->str = null;
			state = sc & 4 ? XNULLSUB : XSUB;
		} else {
			/* yes → load first */
			xp->str = *xp->u.strv++;
			/* $@ or ${foo[@]} */
			xp->split = isWahr(sc & 4);
			state = XARG;
		}
	}

	c = stype & STYPE_CHAR;
	/* test the compiler's code generator */
	if ((!(stype & STYPE_AT) && (ctype(c, C_SUB2) ||
	    (((stype & STYPE_DBL) ? *xp->str == '\0' : xp->str == null) &&
	    (state != XARG || (ifs0 || xp->split ?
	    (xp->u.strv[0] == NULL) : !hasnonempty(xp->u.strv))) ?
	    ctype(c, C_EQUAL | C_MINUS | C_QUEST) : c == ORD('+')))) ||
	    stype == (ORD('0') | STYPE_DBL) ||
	    stype == (ORD('#') | STYPE_AT) ||
	    stype == (ORD('Q') | STYPE_AT) ||
	    stype == (ORD('^') | STYPE_AT) ||
	    (stype & STYPE_CHAR) == ORD('/'))
		/* expand word instead of variable value */
		state = XBASE;
	if (Flag(FNOUNSET) && xp->str == null && !zero_ok &&
	    (ctype(c, C_SUB2) || (state != XBASE && c != ORD('+'))))
		kerrf(KWF_ERR(1) | KWF_PREFIX | KWF_FILELINE | KWF_TWOMSG |
		    KWF_NOERRNO, sp, Tf_parm);
	*stypep = stype;
	*slenp = slen;
	return (state);
}

/*
 * Run the command in $(...) and read its output.
 */
static int
comsub(Expand *xp, const char *cp, int fn)
{
	Source *s, *sold;
	struct op *t;
	struct shf *shf;
	Wahr doalias = Nee;
	kby old_utfmode = UTFMODE;

	switch (fn) {
	case COMASUB:
		fn = COMSUB;
		if (0)
			/* FALLTHROUGH */
	case FUNASUB:
		  fn = FUNSUB;
		doalias = Ja;
	}

	s = pushs(SSTRING, ATEMP);
	s->start = s->str = cp;
	sold = source;
	t = compile(s, doalias);
	afree(s, ATEMP);
	source = sold;

	UTFMODE = old_utfmode;

	if (t == NULL)
		return (XBASE);

	/* no waitlast() unless specifically enabled later */
	xp->split = Nee;

	if (t->type == TCOM &&
	    *t->args == NULL && *t->vars == NULL && t->ioact != NULL) {
		/* $(<file) */
		struct ioword *io = *t->ioact;
		char *name;

		switch (io->ioflag & IOTYPE) {
		case IOREAD:
			shf = shf_open(name = evalstr(io->ioname, DOTILDE),
				O_RDONLY, 0, SHF_MAPHI | SHF_CLEXEC);
			if (shf == NULL)
				kwarnf(KWF_PREFIX | (Flag(FTALKING) ?
				    KWF_FILELINE : 0) | KWF_TWOMSG,
				    name, Tcant_filesub);
			break;
		case IOHERE:
			if (!herein(io, &name)) {
				xp->str = name;
				/* as $(…) requires, trim trailing newlines */
				name = strnul(name);
				while (name > xp->str && name[-1] == '\n')
					--name;
				*name = '\0';
				return (XSUB);
			}
			shf = NULL;
			break;
		default:
			kerrf(KWF_ERR(1) | KWF_PREFIX | KWF_FILELINE |
			    KWF_TWOMSG | KWF_NOERRNO, T_funny_command,
			    snptreef(NULL, 32, Tft_R, io));
		}
	} else if (fn == FUNSUB) {
		int ofd1;
		struct temp *tf = NULL;

		/*
		 * create a temporary file, open for reading and writing,
		 * with an shf open for reading (buffered) but yet unused
		 */
		maketemp(ATEMP, TT_FUNSUB, &tf);
		if (!tf->shf)
			kerrf0(KWF_ERR(1) | KWF_PREFIX | KWF_FILELINE,
			    Tf_temp, Tcreate, tf->tffn);
		/* extract shf from temporary file, unlink and free it */
		shf = tf->shf;
		unlink(tf->tffn);
		afree(tf, ATEMP);
		/* save stdout and let it point to the tempfile */
		ofd1 = savefd(1);
		ksh_dup2(shf_fileno(shf), 1, Nee);
		/*
		 * run tree, with output thrown into the tempfile,
		 * in a new function block
		 */
		funsub(t);
		subst_exstat = exstat & 0xFF;
		/* rewind the tempfile and restore regular stdout */
		lseek(shf_fileno(shf), (off_t)0, SEEK_SET);
		restfd(1, ofd1);
	} else if (fn == VALSUB) {
		xp->str = valsub(t, ATEMP);
		subst_exstat = exstat & 0xFF;
		return (XSUB);
	} else {
		int ofd1, pv[2];

		openpipe(pv);
		shf = shf_fdopen(pv[0], SHF_RD, NULL);
		ofd1 = savefd(1);
		if (pv[1] != 1) {
			ksh_dup2(pv[1], 1, Nee);
			close(pv[1]);
		}
		execute(t, XXCOM | XPIPEO | XFORK, NULL);
		restfd(1, ofd1);
		startlast();
		/* waitlast() */
		xp->split = Ja;
	}

	xp->u.shf = shf;
	return (XCOM);
}

/*
 * perform #pattern and %pattern substitution in ${}
 */
static char *
trimsub(char *str, char *pat, int how)
{
	char *end = strnul(str);
	char *p, c;

	switch (how & (STYPE_CHAR | STYPE_DBL)) {
	case ORD('#'):
		/* shortest match at beginning */
		p = str;
		do {
			c = *p;
			*p = '\0';
			if (gmatchx(str, pat, Nee)) {
				record_match(str);
				*p = c;
				return (p);
			}
			*p = c;
			p += ez_mbtoc(NULL, p);
		} while (c != '\0');
		break;
	case ORD('#') | STYPE_DBL:
		/* longest match at beginning */
		p = end;
		while (p >= str) {
			c = *p;
			*p = '\0';
			if (gmatchx(str, pat, Nee)) {
				record_match(str);
				*p = c;
				return (p);
			}
			*p = c;
			--p;
		}
		break;
	case ORD('%'):
		/* shortest match at end */
		p = end;
		while (p >= str) {
			if (gmatchx(p, pat, Nee))
				goto trimsub_match;
			p = ez_bs(p - 1, str);
		}
		break;
	case ORD('%') | STYPE_DBL:
		/* longest match at end */
		for (p = str; p <= end; p++)
			if (gmatchx(p, pat, Nee)) {
 trimsub_match:
				record_match(p);
				strndupx(end, str, p - str, ATEMP);
				return (end);
			}
		break;
	}

	/* no match, return string */
	return (str);
}

/*
 * glob
 * Name derived from V6's /etc/glob, the program that expanded filenames.
 */

/* XXX cp not const 'cause slashes are temporarily replaced with NULs... */
static void
glob(char *cp, XPtrV *wp, Wahr markdirs)
{
	int oldsize = XPsize(*wp);

	if (glob_str(cp, wp, markdirs) == 0)
		XPput(*wp, debunk(cp, cp, strlen(cp) + 1));
	else
		qsort(XPptrv(*wp) + oldsize, XPsize(*wp) - oldsize,
		    sizeof(void *), ascpstrcmp);
}

#define GF_NONE		0
#define GF_EXCHECK	BIT(0)		/* do existence check on file */
#define GF_GLOBBED	BIT(1)		/* some globbing has been done */
#define GF_MARKDIR	BIT(2)		/* add trailing / to directories */

/*
 * Apply file globbing to cp and store the matching files in wp. Returns
 * the number of matches found.
 */
int
glob_str(char *cp, XPtrV *wp, Wahr markdirs)
{
	int oldsize = XPsize(*wp);
	XString xs;
	char *xp;

	Xinit(xs, xp, 256, ATEMP);
	globit(&xs, &xp, cp, wp, markdirs ? GF_MARKDIR : GF_NONE);
	Xfree(xs, xp);

	return (XPsize(*wp) - oldsize);
}

static void
globit(XString *xs,	/* dest string */
    char **xpp,		/* ptr to dest end */
    char *sp,		/* source path */
    XPtrV *wp,		/* output list */
    int check)		/* GF_* flags */
{
	char *np;		/* next source component */
	char *xp = *xpp;
	char *se;
	char odirsep;
	DIR *dirp;
	size_t prefix_len;

	/* This to allow long expansions to be interrupted */
	intrcheck();

	if (sp == NULL) {
		/* end of source path */
		/*
		 * We only need to check if the file exists if a pattern
		 * is followed by a non-pattern (eg, foo*x/bar; no check
		 * is needed for foo* since the match must exist) or if
		 * any patterns were expanded and the markdirs option is set.
		 * Symlinks make things a bit tricky...
		 */
		if ((check & GF_EXCHECK) ||
		    ((check & GF_MARKDIR) && (check & GF_GLOBBED))) {
#define stat_check()	(stat_done ? stat_done : (stat_done = \
			    stat(Xstring(*xs, xp), &statb) < 0 ? -1 : 1))
			struct stat lstatb, statb;
			/* -1: failed, 1 ok, 0 not yet done */
			int stat_done = 0;

			if (mksh_lstat(Xstring(*xs, xp), &lstatb) < 0)
				return;
			/*
			 * special case for systems which strip trailing
			 * slashes from regular files (eg, /etc/passwd/).
			 * SunOS 4.1.3 does this...
			 */
			if ((check & GF_EXCHECK) && xp > Xstring(*xs, xp) &&
			    mksh_cdirsep(xp[-1]) && !S_ISDIR(lstatb.st_mode) &&
			    (!S_ISLNK(lstatb.st_mode) ||
			    stat_check() < 0 || !S_ISDIR(statb.st_mode)))
				return;
			/*
			 * Possibly tack on a trailing / if there isn't already
			 * one and if the file is a directory or a symlink to a
			 * directory
			 */
			if (((check & GF_MARKDIR) && (check & GF_GLOBBED)) &&
			    xp > Xstring(*xs, xp) && !mksh_cdirsep(xp[-1]) &&
			    (S_ISDIR(lstatb.st_mode) ||
			    (S_ISLNK(lstatb.st_mode) && stat_check() > 0 &&
			    S_ISDIR(statb.st_mode)))) {
				*xp++ = '/';
				*xp = '\0';
			}
		}
		strndupx(np, Xstring(*xs, xp), Xlength(*xs, xp), ATEMP);
		XPput(*wp, np);
		return;
	}

	if (xp > Xstring(*xs, xp))
		*xp++ = '/';
	while (mksh_cdirsep(*sp)) {
		Xcheck(*xs, xp);
		*xp++ = *sp++;
	}
	*xp = '\0';
	np = mksh_sdirsep(sp);
	if (np != NULL) {
		se = np;
		/* don't assume '/', can be multiple kinds */
		odirsep = *np;
		*np++ = '\0';
	} else {
		odirsep = '\0'; /* keep gcc quiet */
		se = strnul(sp);
	}

	/*
	 * Check if sp needs globbing - done to avoid pattern checks for strings
	 * containing MAGIC characters, open [s without the matching close ],
	 * etc. (otherwise opendir() will be called which may fail because the
	 * directory isn't readable - if no globbing is needed, only execute
	 * permission should be required (as per POSIX)).
	 */
	if (!has_globbing(sp)) {
		XcheckN(*xs, xp, se - sp + 1);
		debunk(xp, sp, Xnleft(*xs, xp));
		xp = strnul(xp);
		*xpp = xp;
		globit(xs, xpp, np, wp, check);
	} else if ((dirp = opendir((prefix_len = Xlength(*xs, xp)) ?
	    Xstring(*xs, xp) : Tdot))) {
		struct dirent *d;

		while ((d = readdir(dirp)) != NULL) {
			size_t len;

			if (isch(d->d_name[0], '.') && (!d->d_name[1] ||
			    (isch(d->d_name[1], '.') && !d->d_name[2])))
				/* always ignore . and .. */
				continue;
			if ((isch(d->d_name[0], '.') && !isch(*sp, '.')) ||
			    !gmatchx(d->d_name, sp, Ja))
				continue;

			len = strlen(d->d_name) + 1;
			XcheckN(*xs, xp, len);
			memcpy(xp, d->d_name, len);
			*xpp = xp + len - 1;
			globit(xs, xpp, np, wp, (check & GF_MARKDIR) |
			    GF_GLOBBED | (np ? GF_EXCHECK : GF_NONE));
			xp = Xstring(*xs, xp) + prefix_len;
		}
		closedir(dirp);
	}

	if (np != NULL)
		*(char *)(--np) = odirsep;
}

/* remove MAGIC from string */
char *
debunk(char *dp, const char *sp, size_t dlen)
{
	char *d;
	const char *s;

	if ((s = cstrchr(sp, MAGIC))) {
		if (s - sp >= (ssize_t)dlen)
			return (dp);
		memmove(dp, sp, s - sp);
		for (d = dp + (s - sp); *s && (d - dp < (ssize_t)dlen); s++)
			if (!ISMAGIC(*s) || !(*++s & 0x80) ||
			    !ctype(*s & 0x7F, C_PATMO | C_SPC))
				*d++ = *s;
			else {
				/* extended pattern operators: *+?@! */
				if ((*s & 0x7f) != ' ')
					*d++ = *s & 0x7f;
				if (d - dp < (ssize_t)dlen)
					*d++ = '(';
			}
		*d = '\0';
	} else if (dp != sp)
		strlcpy(dp, sp, dlen);
	return (dp);
}

/*
 * Check if p is an unquoted name, possibly followed by a / or :. If so
 * puts the expanded version in *dcp,dp and returns a pointer in p just
 * past the name, otherwise returns 0.
 */
static const char *
maybe_expand_tilde(const char *p, XString *dsp, char **dpp, Wahr isassign)
{
	XString ts;
	char *dp = *dpp;
	char *tp;
	const char *r;

	Xinit(ts, tp, 16, ATEMP);
	/* : only for DOASNTILDE form */
	while (p[0] == CHAR && /* not cdirsep */ p[1] != '/' &&
	    (!isassign || p[1] != ':')) {
		Xcheck(ts, tp);
		*tp++ = p[1];
		p += 2;
	}
	*tp = '\0';
	r = (p[0] == EOS || p[0] == CHAR || p[0] == CSUBST) ?
	    do_tilde(Xstring(ts, tp)) : NULL;
	Xfree(ts, tp);
	if (r) {
		while (*r) {
			Xcheck(*dsp, dp);
			if (ISMAGIC(*r))
				*dp++ = MAGIC;
			*dp++ = *r++;
		}
		*dpp = dp;
		r = p;
	}
	return (r);
}

/*
 * tilde expansion
 *
 * based on a version by Arnold Robbins
 */
char *
do_tilde(char *cp)
{
	char *dp = null;
#ifndef MKSH_NOPWNAM
	Wahr do_simplify = Ja;
#endif

	if (cp[0] == '\0')
		dp = str_val(global("HOME"));
	else if (cp[0] == '+' && cp[1] == '\0')
		dp = str_val(global(TPWD));
	else if (ksh_isdash(cp))
		dp = str_val(global(TOLDPWD));
#ifndef MKSH_NOPWNAM
	else {
		dp = homedir(cp);
		do_simplify = Nee;
	}
#endif

	/* if parameters aren't set, don't expand ~ */
	if (dp == NULL || dp == null)
		return (NULL);

	/* simplify parameters as if cwd upon entry */
#ifndef MKSH_NOPWNAM
	if (do_simplify)
#endif
	  {
		strdupx(dp, dp, ATEMP);
		simplify_path(dp);
	}
	return (dp);
}

#ifndef MKSH_NOPWNAM
/*
 * map userid to user's home directory.
 * note that 4.3's getpw adds more than 6K to the shell,
 * and the YP version probably adds much more.
 * we might consider our own version of getpwnam() to keep the size down.
 */
static char *
homedir(char *name)
{
	struct tbl *ap;

	ap = ktenter(&homedirs, name, hash(name));
	if (!(ap->flag & ISSET)) {
		struct passwd *pw;

		pw = getpwnam(name);
		if (pw == NULL)
			return (NULL);
		strdupx(ap->val.s, pw->pw_dir, APERM);
		ap->flag |= DEFINED|ISSET|ALLOC;
	}
	return (ap->val.s);
}
#endif

static void
alt_expand(XPtrV *wp, char *start, char *exp_start, char *end, int fdo)
{
	unsigned int count = 0;
	char *brace_start, *brace_end, *comma = NULL;
	char *field_start;
	char *p = exp_start;

	/* search for open brace */
	while ((p = ucstrchr(p, MAGIC)) && ord(p[1]) != ORD('{' /*}*/))
		p += 2;
	brace_start = p;

	/* find matching close brace, if any */
	if (p) {
		comma = NULL;
		count = 1;
		p += 2;
		while (*p && count) {
			if (ISMAGIC(*p++)) {
				if (ord(*p) == ORD('{' /*}*/))
					++count;
				else if (ord(*p) == ORD(/*{*/ '}'))
					--count;
				else if (*p == ',' && count == 1)
					comma = p;
				++p;
			}
		}
	}
	/* no valid expansions... */
	if (!p || count != 0) {
		/*
		 * Note that given a{{b,c} we do not expand anything (this is
		 * what AT&T ksh does. This may be changed to do the {b,c}
		 * expansion. }
		 */
		if (fdo & DOGLOB)
			glob(start, wp, isWahr(fdo & DOMARKDIRS));
		else
			XPput(*wp, debunk(start, start, end - start));
		return;
	}
	brace_end = p;
	if (!comma) {
		alt_expand(wp, start, brace_end, end, fdo);
		return;
	}

	/* expand expression */
	field_start = brace_start + 2;
	count = 1;
	for (p = brace_start + 2; p != brace_end; p++) {
		if (ISMAGIC(*p)) {
			if (ord(*++p) == ORD('{' /*}*/))
				++count;
			else if ((ord(*p) == ORD(/*{*/ '}') && --count == 0) ||
			    (*p == ',' && count == 1)) {
				char *news;
				int l1, l2, l3;

				/*
				 * addition safe since these operate on
				 * one string (separate substrings)
				 */
				l1 = brace_start - start;
				l2 = (p - 1) - field_start;
				l3 = end - brace_end;
				news = alloc(l1 + l2 + l3 + 1, ATEMP);
				memcpy(news, start, l1);
				memcpy(news + l1, field_start, l2);
				memcpy(news + l1 + l2, brace_end, l3);
				news[l1 + l2 + l3] = '\0';
				alt_expand(wp, news, news + l1,
				    news + l1 + l2 + l3, fdo);
				field_start = p + 1;
			}
		}
	}
	return;
}

/* helper function due to setjmp/longjmp woes */
static char *
valsub(struct op *t, Area *ap)
{
	char *cp;
	struct tbl * volatile vp;
	struct block * volatile l;
	int i;

	l = e->loc;
	newenv(E_FUNC);
	newblock();
	vp = local(TREPLY, Nee);
	if (!(i = kshsetjmp(e->jbuf))) {
		execute(t, XXCOM, NULL);
		i = LRETURN;
	}
	l->argc = e->loc->argc;
	l->argv = e->loc->argv;
	strdupx(cp, str_val(vp), ap);
	quitenv(NULL);
	/* see CFUNC case in exec.c:comexec() */
	if (i != LRETURN)
		unwind(i);

	return (cp);
}

static void
funsub(struct op *t)
{
	struct block * volatile l;

	l = e->loc;
	newenv(E_FUNC);
	newblock();
	if (!kshsetjmp(e->jbuf))
		execute(t, XXCOM | XERROK, NULL);
	l->argc = e->loc->argc;
	l->argv = e->loc->argv;
	quitenv(NULL);
}
