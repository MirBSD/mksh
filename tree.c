/*	$OpenBSD: tree.c,v 1.21 2015/09/01 13:12:31 tedu Exp $	*/

/*-
 * Copyright (c) 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010,
 *		 2011, 2012, 2013, 2015, 2016, 2017, 2021
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

__RCSID("$MirOS: src/bin/mksh/tree.c,v 1.114 2024/07/26 18:34:58 tg Exp $");

#define INDENT	8

static void ptree(struct op *, int, struct shf *);
static void pioact(struct shf *, struct ioword *);
static const char *wdvarput(struct shf *, const char *, int, int);
static void vfptreef(struct shf *, int, const char *, va_list);
static struct ioword **iocopy(struct ioword **, Area *);
static void iofree(struct ioword **, Area *);

/* "foo& ; bar" and "foo |& ; bar" are invalid */
static Wahr prevent_semicolon;

/* here document diversion */
static unsigned short ptree_nest;
static Wahr ptree_hashere;
static struct shf ptree_heredoc;
#define ptree_outhere(shf) do {					\
	if (ptree_hashere) {					\
		char *ptree_thehere;				\
								\
		ptree_thehere = shf_sclose(&ptree_heredoc);	\
		shf_puts(ptree_thehere, (shf));			\
		shf_putc('\n', (shf));				\
		afree(ptree_thehere, ATEMP);			\
		ptree_hashere = Nee;				\
		/*prevent_semicolon = Ja;*/			\
	}							\
} while (/* CONSTCOND */ 0)

static const char Telif_pT[] = "elif %T";

/*
 * print a command tree
 */
static void
ptree(struct op *t, int indent, struct shf *shf)
{
	const char **w;
	struct ioword **ioact;
	struct op *t1;
	int i;
	const char *ccp;

 Chain:
	if (t == NULL)
		return;
	switch (t->type) {
	case TCOM:
		prevent_semicolon = Nee;
		/* special-case 'var=<<EOF' (cf. exec.c:execute) */
		if (t->args &&
		    /* we have zero arguments, i.e. no program to run */
		    t->args[0] == NULL &&
		    /* we have exactly one variable assignment */
		    t->vars[0] != NULL && t->vars[1] == NULL &&
		    /* we have exactly one I/O redirection */
		    t->ioact != NULL && t->ioact[0] != NULL &&
		    t->ioact[1] == NULL &&
		    /* of type "here document" (or "here string") */
		    (t->ioact[0]->ioflag & IOTYPE) == IOHERE &&
		    /* the variable assignment begins with a valid varname */
		    (ccp = skip_wdvarname(t->vars[0], Ja)) != t->vars[0] &&
		    /* and has no right-hand side (i.e. "varname=") */
		    ccp[0] == CHAR && ((ccp[1] == '=' && ccp[2] == EOS) ||
		    /* or "varname+=" */ (ccp[1] == '+' && ccp[2] == CHAR &&
		    ccp[3] == '=' && ccp[4] == EOS))) {
			fptreef(shf, indent, Tf_S, t->vars[0]);
			break;
		}

		if (t->vars) {
			w = (const char **)t->vars;
			while (*w)
				fptreef(shf, indent, Tf_S_, *w++);
		}
#ifndef MKSH_SMALL
		  else
			shf_puts("#no-vars# ", shf);
#endif
		if (t->args) {
			w = t->args;
			if (*w && **w == CHAR) {
				char *cp = wdstrip(*w++, WDS_TPUTS);

				if (valid_alias_name(cp))
					shf_putc('\\', shf);
				shf_puts(cp, shf);
				shf_putc(' ', shf);
				afree(cp, ATEMP);
			}
			while (*w)
				fptreef(shf, indent, Tf_S_, *w++);
		}
#ifndef MKSH_SMALL
		  else
			shf_puts("#no-args# ", shf);
#endif
		break;
	case TEXEC:
		t = t->left;
		goto Chain;
	case TPAREN:
		fptreef(shf, indent + 2, "( %T) ", t->left);
		break;
	case TPIPE:
		fptreef(shf, indent, "%T| ", t->left);
		t = t->right;
		goto Chain;
	case TLIST:
		fptreef(shf, indent, "%T%;", t->left);
		t = t->right;
		goto Chain;
	case TOR:
	case TAND:
		fptreef(shf, indent, "%T%s %T",
		    t->left, (t->type == TOR) ? "||" : "&&", t->right);
		break;
	case TBANG:
		shf_puts("! ", shf);
		prevent_semicolon = Nee;
		t = t->right;
		goto Chain;
	case TDBRACKET:
		w = t->args;
		shf_puts("[[", shf);
		while (*w)
			fptreef(shf, indent, Tf__S, *w++);
		shf_puts(" ]] ", shf);
		break;
	case TSELECT:
	case TFOR:
		fptreef(shf, indent, "%s %s ",
		    (t->type == TFOR) ? "for" : Tselect, t->str);
		if (t->vars != NULL) {
			shf_puts("in ", shf);
			w = (const char **)t->vars;
			while (*w)
				fptreef(shf, indent, Tf_S_, *w++);
			fptreef(shf, indent, Tft_end);
		}
		fptreef(shf, indent + INDENT, "do%N%T", t->left);
		fptreef(shf, indent, "%;done ");
		break;
	case TCASE:
		fptreef(shf, indent, "case %S in", t->str);
		for (t1 = t->left; t1 != NULL; t1 = t1->right) {
			fptreef(shf, indent, "%N(");
			w = (const char **)t1->vars;
			while (*w) {
				fptreef(shf, indent, "%S%c", *w,
				    (w[1] != NULL) ? '|' : ')');
				++w;
			}
			fptreef(shf, indent + INDENT, "%N%T%N;%c", t1->left,
			    t1->u.charflag);
		}
		fptreef(shf, indent, "%Nesac ");
		break;
	case TELIF:
		kerrf(KWF_INTERNAL | KWF_ERR(0xFF) | KWF_ONEMSG | KWF_NOERRNO,
		    TELIF_unexpected);
		/* FALLTHROUGH */
	case TIF:
		i = 2;
		t1 = t;
		goto process_TIF;
		do {
			t1 = t1->right;
			i = 0;
			fptreef(shf, indent, Tft_end);
 process_TIF:
			/* 5 == strlen("elif ") */
			fptreef(shf, indent + 5 - i, Telif_pT + i, t1->left);
			t1 = t1->right;
			if (t1->left != NULL) {
				fptreef(shf, indent, Tft_end);
				fptreef(shf, indent + INDENT, "%s%N%T",
				    "then", t1->left);
			}
		} while (t1->right && t1->right->type == TELIF);
		if (t1->right != NULL) {
			fptreef(shf, indent, Tft_end);
			fptreef(shf, indent + INDENT, "%s%N%T",
			    "else", t1->right);
		}
		fptreef(shf, indent, "%;fi ");
		break;
	case TWHILE:
	case TUNTIL:
		/* 6 == strlen("while "/"until ") */
		fptreef(shf, indent + 6, Tf_s_T,
		    (t->type == TWHILE) ? "while" : "until",
		    t->left);
		fptreef(shf, indent, Tft_end);
		fptreef(shf, indent + INDENT, "do%N%T", t->right);
		fptreef(shf, indent, "%;done ");
		break;
	case TBRACE:
		fptreef(shf, indent + INDENT, "{%N%T", t->left);
		fptreef(shf, indent, "%;} ");
		break;
	case TCOPROC:
		fptreef(shf, indent, "%T|& ", t->left);
		prevent_semicolon = Ja;
		break;
	case TASYNC:
		fptreef(shf, indent, "%T& ", t->left);
		prevent_semicolon = Ja;
		break;
	case TFUNCT:
		fpFUNCTf(shf, indent, isWahr(t->u.ksh_func), t->str, t->left);
		break;
	case TTIME:
		fptreef(shf, indent, Tf_s_T, Ttime, t->left);
		break;
	default:
		shf_puts("<botch>", shf);
		prevent_semicolon = Nee;
		break;
	}
	if ((ioact = t->ioact) != NULL)
		while (*ioact != NULL)
			pioact(shf, *ioact++);
}

static void
pioact(struct shf *shf, struct ioword *iop)
{
	unsigned short flag = iop->ioflag;
	unsigned short type = flag & IOTYPE;
	short expected;

	expected = (type == IOREAD || type == IORDWR || type == IOHERE) ? 0 :
	    (type == IOCAT || type == IOWRITE) ? 1 :
	    (type == IODUP && (iop->unit == !(flag & IORDUP))) ? iop->unit :
	    iop->unit + 1;
	if (iop->unit != expected)
		shf_fprintf(shf, Tf_d, (int)iop->unit);

	switch (type) {
	case IOREAD:
		shf_putc('<', shf);
		break;
	case IOHERE:
		if (flag & IOHERESTR) {
			shf_puts("<<<", shf);
			goto ioheredelim;
		}
		shf_puts("<<", shf);
		if (flag & IOSKIP)
			shf_putc('-', shf);
		if (iop->heredoc /* nil when tracing */) {
			/* here document diversion */
			if (!ptree_hashere) {
				shf_sopen(NULL, 0, SHF_WR | SHF_DYNAMIC,
				    &ptree_heredoc);
				ptree_hashere = Ja;
			}
			shf_putc('\n', &ptree_heredoc);
			shf_puts(iop->heredoc, &ptree_heredoc);
			/* iop->delim is set before iop->heredoc */
			shf_putsv(evalstr(iop->delim, 0), &ptree_heredoc);
		}
 ioheredelim:
		/* delim is NULL during syntax error printing */
		if (iop->delim && !(iop->ioflag & IONDELIM))
			wdvarput(shf, iop->delim, 0, WDS_TPUTS);
		break;
	case IOCAT:
		shf_puts(">>", shf);
		break;
	case IOWRITE:
		shf_putc('>', shf);
		if (flag & IOCLOB)
			shf_putc('|', shf);
		break;
	case IORDWR:
		shf_puts("<>", shf);
		break;
	case IODUP:
		shf_putc(flag & IORDUP ? '<' : '>', shf);
		shf_putc('&', shf);
		break;
	}
	/* name is NULL for IOHERE or when printing syntax errors */
	if (iop->ioname) {
		if (flag & IONAMEXP)
			print_value_quoted(shf, iop->ioname);
		else
			wdvarput(shf, iop->ioname, 0, WDS_TPUTS);
	}
	shf_putc(' ', shf);
	prevent_semicolon = Nee;
}

/* variant of fputs for ptreef and wdstrip */
static const char *
wdvarput(struct shf *shf, const char *wp, int quotelevel, int opmode)
{
	int c;
	const char *cs;

	/*-
	 * problems:
	 *	`...` -> $(...)
	 *	'foo' -> "foo"
	 *	x${foo:-"hi"} -> x${foo:-hi} unless WDS_TPUTS
	 *	x${foo:-'hi'} -> x${foo:-hi}
	 * could change encoding to:
	 *	OQUOTE ["'] ... CQUOTE ["']
	 *	COMSUB [(`] ...\0	(handle $ ` \ and maybe " in `...` case)
	 */
	while (/* CONSTCOND */ 1)
		switch (*wp++) {
		case EOS:
			return (--wp);
		case ADELIM:
			if (ord(*wp) == ORD(/*{*/ '}')) {
				++wp;
				goto wdvarput_csubst;
			}
			/* FALLTHROUGH */
		case CHAR:
			c = ord(*wp++);
			shf_putc(c, shf);
			break;
		case QCHAR:
			c = ord(*wp++);
			if (opmode & WDS_TPUTS)
				switch (c) {
				default:
					if (quotelevel == 0)
						/* FALLTHROUGH */
				case ORD('"'):
				case ORD('`'):
				case ORD('$'):
				case ORD('\\'):
					  shf_putc(ORD('\\'), shf);
					break;
				}
			shf_putc(c, shf);
			break;
		case COMASUB:
		case COMSUB:
			shf_puts("$(", shf);
			cs = ")";
			if (ord(*wp) == ORD('(' /*)*/))
				shf_putc(' ', shf);
 pSUB:
			while ((c = *wp++) != 0)
				shf_putc(c, shf);
			shf_puts(cs, shf);
			break;
		case FUNASUB:
		case FUNSUB:
			c = ORD(' ');
			if (0)
				/* FALLTHROUGH */
		case VALSUB:
			  c = ORD('|');
			shf_putc('$', shf);
			shf_putc('{', shf);
			shf_putc(c, shf);
			cs = ";}";
			goto pSUB;
		case EXPRSUB:
			shf_puts("$((", shf);
			cs = "))";
			goto pSUB;
		case OQUOTE:
			if (opmode & WDS_TPUTS) {
				quotelevel++;
				shf_putc('"', shf);
			}
			break;
		case CQUOTE:
			if (opmode & WDS_TPUTS) {
				if (quotelevel)
					quotelevel--;
				shf_putc('"', shf);
			}
			break;
		case OSUBST:
			shf_putc('$', shf);
			if (ord(*wp++) == ORD('{'))
				shf_putc('{', shf);
			while ((c = *wp++) != 0)
				shf_putc(c, shf);
			wp = wdvarput(shf, wp, 0, opmode);
			break;
		case CSUBST:
			if (ord(*wp++) == ORD('}')) {
 wdvarput_csubst:
				shf_putc('}', shf);
			}
			return (wp);
		case OPAT:
			c = *wp++;
			shf_putc(c, shf);
			shf_putc('(', shf);
			break;
		case SPAT:
			c = ORD('|');
			if (0)
				/* FALLTHROUGH */
		case CPAT:
			  c = ORD(/*(*/ ')');
			shf_putc(c, shf);
			break;
		}
}

/*
 * this is the _only_ way to reliably handle
 * variable args with an ANSI compiler
 */
/* VARARGS */
void
fptreef(struct shf *shf, int indent, const char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	vfptreef(shf, indent, fmt, va);
	va_end(va);
}

/* VARARGS */
char *
snptreef(char *s, ssize_t n, const char *fmt, ...)
{
	va_list va;
	struct shf shf;

	shf_sopen(s, n, SHF_WR | (s ? 0 : SHF_DYNAMIC), &shf);

	va_start(va, fmt);
	vfptreef(&shf, 0, fmt, va);
	va_end(va);

	/* shf_sclose NUL terminates */
	return (shf_sclose(&shf));
}

static void
vfptreef(struct shf *shf, int indent, const char *fmt, va_list va)
{
	int c;

	if (!ptree_nest++)
		ptree_hashere = Nee;

	while ((c = ord(*fmt++))) {
		if (c == '%') {
			switch ((c = ord(*fmt++))) {
			case ORD('s'):
				/* string */
				shf_putsv(va_arg(va, char *), shf);
				break;
			case ORD('S'):
				/* word */
				wdvarput(shf, va_arg(va, char *), 0, WDS_TPUTS);
				break;
			case ORD('d'):
				/* signed decimal */
				shf_fprintf(shf, Tf_d, va_arg(va, int));
				break;
			case ORD('u'):
				/* unsigned decimal */
				shf_fprintf(shf, "%u", va_arg(va, unsigned int));
				break;
			case ORD('T'):
				/* format tree */
				ptree(va_arg(va, struct op *), indent, shf);
				goto dont_trash_prevent_semicolon;
			case ORD(';'):
				/* newline or ; */
			case ORD('N'):
				/* newline or space */
				if (shf->flags & SHF_STRING) {
					if ((unsigned int)c == ORD(';') &&
					    !prevent_semicolon)
						shf_putc(';', shf);
					shf_putc(' ', shf);
				} else {
					int i = indent;

					ptree_outhere(shf);
					shf_putc('\n', shf);
					while (i >= 8) {
						shf_putc('\t', shf);
						i -= 8;
					}
					while (i--)
						shf_putc(' ', shf);
				}
				break;
			case ORD('R'):
				/* I/O redirection */
				pioact(shf, va_arg(va, struct ioword *));
				break;
			case ORD('c'):
				/* character (octet, probably) */
				c = va_arg(va, int);
				/* FALLTHROUGH */
			default:
				shf_putc(c, shf);
				break;
			}
		} else
			shf_putc(c, shf);
		prevent_semicolon = Nee;
 dont_trash_prevent_semicolon:
		;
	}

	if (!--ptree_nest)
		ptree_outhere(shf);
}

/*
 * copy tree (for function definition)
 */
struct op *
tcopy(struct op *t, Area *ap)
{
	struct op *r;
	const char **tw;
	char **rw;

	if (t == NULL)
		return (NULL);

	r = alloc(sizeof(struct op), ap);

	r->type = t->type;
	r->u.evalflags = t->u.evalflags;

	if (t->type == TCASE)
		r->str = wdcopy(t->str, ap);
	else
		strdupx(r->str, t->str, ap);

	if (t->vars == NULL)
		r->vars = NULL;
	else {
		tw = (const char **)t->vars;
		while (*tw)
			++tw;
		rw = r->vars = alloc2(tw - (const char **)t->vars + 1,
		    sizeof(*tw), ap);
		tw = (const char **)t->vars;
		while (*tw)
			*rw++ = wdcopy(*tw++, ap);
		*rw = NULL;
	}

	if (t->args == NULL)
		r->args = NULL;
	else {
		tw = t->args;
		while (*tw)
			++tw;
		r->args = (const char **)(rw = alloc2(tw - t->args + 1,
		    sizeof(*tw), ap));
		tw = t->args;
		while (*tw)
			*rw++ = wdcopy(*tw++, ap);
		*rw = NULL;
	}

	r->ioact = (t->ioact == NULL) ? NULL : iocopy(t->ioact, ap);

	r->left = tcopy(t->left, ap);
	r->right = tcopy(t->right, ap);
	r->lineno = t->lineno;

	return (r);
}

char *
wdcopy(const char *wp, Area *ap)
{
	size_t len;

	len = wdscan(wp, EOS) - wp;
	return (memcpy(alloc(len, ap), wp, len));
}

/* return the position of prefix c in wp plus 1 */
const char *
wdscan(const char *wp, int c)
{
	int nest = 0;

	while (/* CONSTCOND */ 1)
		switch (*wp++) {
		case EOS:
			return (wp);
		case ADELIM:
			if (c == ADELIM && nest == 0)
				return (wp + 1);
			if (ord(*wp) == ORD(/*{*/ '}'))
				goto wdscan_csubst;
			/* FALLTHROUGH */
		case CHAR:
		case QCHAR:
			wp++;
			break;
		case COMASUB:
		case COMSUB:
		case FUNASUB:
		case FUNSUB:
		case VALSUB:
		case EXPRSUB:
			while (*wp++ != 0)
				;
			break;
		case OQUOTE:
		case CQUOTE:
			break;
		case OSUBST:
			nest++;
			while (*wp++ != '\0')
				;
			break;
		case CSUBST:
 wdscan_csubst:
			wp++;
			if (c == CSUBST && nest == 0)
				return (wp);
			nest--;
			break;
		case OPAT:
			nest++;
			wp++;
			break;
		case SPAT:
		case CPAT:
			if (c == wp[-1] && nest == 0)
				return (wp);
			if (wp[-1] == CPAT)
				nest--;
			break;
		default:
			kwarnf0(KWF_INTERNAL | KWF_WARNING | KWF_NOERRNO,
			    "wdscan: unknown char 0x%X (carrying on)",
			    KBI(wp[-1]));
		}
}

/*
 * return a copy of wp without any of the mark up characters and with
 * quote characters (" ' \) stripped. (string is allocated from ATEMP)
 */
char *
wdstrip(const char *wp, int opmode)
{
	struct shf shf;

	shf_sopen(NULL, 32, SHF_WR | SHF_DYNAMIC, &shf);
	wdvarput(&shf, wp, 0, opmode);
	/* shf_sclose NUL terminates */
	return (shf_sclose(&shf));
}

static struct ioword **
iocopy(struct ioword **iow, Area *ap)
{
	struct ioword **ior;
	int i;

	ior = iow;
	while (*ior)
		++ior;
	ior = alloc2(ior - iow + 1, sizeof(struct ioword *), ap);

	for (i = 0; iow[i] != NULL; i++) {
		struct ioword *p, *q;

		p = iow[i];
		q = alloc(sizeof(struct ioword), ap);
		ior[i] = q;
		*q = *p;
		if (p->ioname != NULL)
			q->ioname = wdcopy(p->ioname, ap);
		if (p->delim != NULL)
			q->delim = wdcopy(p->delim, ap);
		if (p->heredoc != NULL)
			strdupx(q->heredoc, p->heredoc, ap);
	}
	ior[i] = NULL;

	return (ior);
}

/*
 * free tree (for function definition)
 */
void
tfree(struct op *t, Area *ap)
{
	char **w;

	if (t == NULL)
		return;

	afree(t->str, ap);

	if (t->vars != NULL) {
		for (w = t->vars; *w != NULL; w++)
			afree(*w, ap);
		afree(t->vars, ap);
	}

	if (t->args != NULL) {
		/*XXX we assume the caller is right */
		union mksh_ccphack cw;

		cw.ro = t->args;
		for (w = cw.rw; *w != NULL; w++)
			afree(*w, ap);
		afree(t->args, ap);
	}

	if (t->ioact != NULL)
		iofree(t->ioact, ap);

	tfree(t->left, ap);
	tfree(t->right, ap);

	afree(t, ap);
}

static void
iofree(struct ioword **iow, Area *ap)
{
	struct ioword **iop;
	struct ioword *p;

	iop = iow;
	while ((p = *iop++) != NULL) {
		afree(p->ioname, ap);
		afree(p->delim, ap);
		afree(p->heredoc, ap);
		afree(p, ap);
	}
	afree(iow, ap);
}

void
fpFUNCTf(struct shf *shf, int i, Wahr isksh, const char *k, struct op *v)
{
	if (isksh)
		fptreef(shf, i, "%s %s %T", Tfunction, k, v);
	else if (ktsearch(&keywords, k, hash(k)))
		fptreef(shf, i, "%s %s() %T", Tfunction, k, v);
	else
		fptreef(shf, i, "%s() %T", k, v);
}

void
uprntc(unsigned char c, struct shf *shf)
{
	unsigned char a;

	if (ctype(c, C_PRINT)) {
 doprnt:
		shf_putc(c, shf);
		return;
	}

	if (!UTFMODE) {
		if (!ksh_isctrl8(c))
			goto doprnt;
		if ((a = rtt2asc(c)) >= 0x80U) {
			shf_scheck(3, shf);
			shf_putc('^', shf);
			shf_putc('!', shf);
			a &= 0x7FU;
			goto unctrl;
		}
	} else if ((a = rtt2asc(c)) >= 0x80U) {
		shf_scheck(4, shf);
		shf_putc('\\', shf);
		shf_putc('x', shf);
		shf_putc(digits_uc[(a >> 4) & 0x0F], shf);
		shf_putc(digits_uc[a & 0x0F], shf);
		return;
	}
	shf_scheck(2, shf);
	shf_putc('^', shf);
 unctrl:
	shf_putc(asc2rtt(a ^ 0x40U), shf);
}

#ifndef MKSH_NO_CMDLINE_EDITING
/*
 * For now, these are only used by the edit code, which requires
 * a difference: tab is output as three spaces, not as control
 * character in caret notation. If these will ever be used else‐
 * where split them up.
 */
/*size_t*/ void
uescmbT(unsigned char *dst, const char **cpp)
{
	unsigned char c;
	unsigned int wc;
	size_t n, dstsz = 0;
	const char *cp = *cpp;

	c = *cp++;
	/* test cheap first (easiest) */
	if (ctype(c, C_PRINT)) {
 prntb:
		dst[dstsz++] = c;
		goto out;
	}
	/* differently from uprntc, note tab as three spaces */
	if (ord(c) == CTRL_I) {
		dst[dstsz++] = ' ';
		dst[dstsz++] = ' ';
		dst[dstsz++] = ' ';
		goto out;
	}

	/* more specialised tests depend on shell state */
	if (UTFMODE) {
		/* wc = rtt2asc(c) except UTF-8 is decoded */
		if ((n = utf_mbtowc(&wc, cp - 1)) == (size_t)-1) {
			/* failed: invalid UTF-8 */
			wc = rtt2asc(c);
			dst[dstsz++] = '\\';
			dst[dstsz++] = 'x';
			dst[dstsz++] = digits_uc[(wc >> 4) & 0x0F];
			dst[dstsz++] = digits_uc[wc & 0x0F];
			goto out;
		}
		/*
		 * printable as-is? U+0020‥U+007E already handled
		 * above as they are C_PRINT, U+00A0 or higher are
		 * not escaped either, anything in between is special
		 */
		if (wc >= 0xA0U) {
			dst[dstsz++] = c;
			switch (n) {
#ifdef notyet
			case 4:
				dst[dstsz++] = *cp++;
				/* FALLTHROUGH */
#endif
			case 3:
				dst[dstsz++] = *cp++;
				/* FALLTHROUGH */
			default:
				dst[dstsz++] = *cp++;
				break;
			}
			goto out;
		}
		/* and encoded with either 1 or 2 octets */

		/* C1 control character, UTF-8 encoded */
		if (wc >= 0x80U) {
			/* n == 2 so we miss one out */
			++cp;

			c = '+';
			goto prntC1;
		}
		/* nope, must be C0 or DEL */
		/* n == 1 so cp needs no adjustment */
		goto prntC0;
	}

	/* not UTFMODE allows more but the test is more expensive */
	if (!ksh_isctrl8(c))
		goto prntb;
	/* UTF-8 is not decoded, we just transfer an octet to ASCII */
	wc = rtt2asc(c);
	/* C1 control character octet? */
	if (wc >= 0x80U) {
		c = '!';
 prntC1:
		dst[dstsz++] = '^';
		dst[dstsz++] = c;
		wc &= 0x7FU;
	} else {
		/* nope, so C0 or DEL, anything else went to prntb */
 prntC0:
		dst[dstsz++] = '^';
	}
	dst[dstsz++] = asc2rtt(wc ^ 0x40U);

 out:
	*cpp = cp;
	dst[dstsz] = '\0';
#ifdef usedoutsideofedit
	return (dstsz);
#endif
}

int
uwidthmbT(char *cp, char **dcp)
{
	unsigned char c;
	unsigned int wc;
	int w;
	size_t n;

	c = *cp++;
	/* test cheap first (easiest) */
	if (ctype(c, C_PRINT)) {
 prntb:
		w = 1;
		goto out;
	}
	/* differently from uprntc, note tab as three spaces */
	if (ord(c) == CTRL_I) {
		w = 3;
		goto out;
	}

	/* more specialised tests depend on shell state */
	if (UTFMODE) {
		/* wc = rtt2asc(c) except UTF-8 is decoded */
		if ((n = utf_mbtowc(&wc, cp - 1)) == (size_t)-1) {
			/* \x## */
			w = 4;
			goto out;
		}
		cp += n - 1;
		/*
		 * printable as-is? U+0020‥U+007E already handled
		 * above as they are C_PRINT, U+00A0 or higher are
		 * not escaped either, anything in between is special
		 */
		if (wc >= 0xA0U) {
			w = utf_wcwidth(wc);
			goto out;
		}
		/* and encoded with either 1 or 2 octets */

		/* C1 control character, UTF-8 encoded */
		if (wc >= 0x80U)
			goto prntC1;
		/* nope, must be C0 or DEL */
		goto prntC0;
	}

	/* not UTFMODE allows more but the test is more expensive */
	if (!ksh_isctrl8(c))
		goto prntb;
	/* UTF-8 is not decoded, we just transfer an octet to ASCII */
	wc = rtt2asc(c);
	/* C1 control character octet? */
	if (wc >= 0x80U) {
 prntC1:
		w = 3;
	} else {
		/* nope, so C0 or DEL, anything else went to prntb */
 prntC0:
		w = 2;
	}

 out:
	if (dcp)
		*dcp = cp;
	return (w);
}
#endif

const char *
uprntmbs(const char *cp, Wahr esc_caret, struct shf *shf)
{
	unsigned char c;
	unsigned int wc;
	size_t n;

	while ((c = *cp++) != 0) {
		/* test cheap first (easiest) */
		if (ctype(c, C_PRINT)) {
			if (esc_caret && (c == ORD('\\') || c == ORD('^'))) {
				shf_scheck(2, shf);
				shf_putc('\\', shf);
			}
 prntb:
			shf_putc(c, shf);
			continue;
		}

		/* more specialised tests depend on shell state */
		if (UTFMODE) {
			/* wc = rtt2asc(c) except UTF-8 is decoded */
			if ((n = utf_mbtowc(&wc, cp - 1)) == (size_t)-1) {
				/* failed: invalid UTF-8 */
				wc = rtt2asc(c);
				shf_scheck(4, shf);
				shf_putc('\\', shf);
				shf_putc('x', shf);
				shf_putc(digits_uc[(wc >> 4) & 0x0F], shf);
				shf_putc(digits_uc[wc & 0x0F], shf);
				continue;
			}
			/*
			 * printable as-is? U+0020‥U+007E already handled
			 * above as they are C_PRINT, U+00A0 or higher are
			 * not escaped either, anything in between is special
			 */
			if (wc >= 0xA0U) {
				--cp;
				shf_wr_sm(cp, n, shf);
				continue;
			}
			/* and encoded with either 1 or 2 octets */

			/* C1 control character, UTF-8 encoded */
			if (wc >= 0x80U) {
				/* n == 2 so we miss one out */
				++cp;

				c = '+';
				goto prntC1;
			}
			/* nope, must be C0 or DEL */
			/* n == 1 so cp needs no adjustment */
			goto prntC0;
		}

		/* not UTFMODE allows more but the test is more expensive */
		if (!ksh_isctrl8(c))
			goto prntb;
		/* UTF-8 is not decoded, we just transfer an octet to ASCII */
		wc = rtt2asc(c);
		/* C1 control character octet? */
		if (wc >= 0x80U) {
			c = '!';
 prntC1:
			shf_scheck(3, shf);
			shf_putc('^', shf);
			shf_putc(c, shf);
			wc &= 0x7FU;
		} else {
			/* nope, so C0 or DEL, anything else went to prntb */
 prntC0:
			shf_scheck(2, shf);
			shf_putc('^', shf);
		}
		shf_putc(asc2rtt(wc ^ 0x40U), shf);
	}
	/* point to the trailing NUL for continuation */
	return ((const void *)(cp - 1));
}

#ifdef DEBUG
/* see: wdvarput */
static const char *
dumpwdvar_i(struct shf *shf, const char *wp, int quotelevel)
{
	int c;

	while (/* CONSTCOND */ 1) {
		switch (*wp++) {
		case EOS:
			shf_puts("EOS", shf);
			return (--wp);
		case ADELIM:
			if (ord(*wp) == ORD(/*{*/ '}')) {
				shf_puts(/*{*/ "]ADELIM(})", shf);
				return (wp + 1);
			}
			shf_puts("ADELIM=", shf);
			if (0)
				/* FALLTHROUGH */
		case CHAR:
			  shf_puts("CHAR=", shf);
			uprntc(*wp++, shf);
			break;
		case QCHAR:
			shf_puts("QCHAR<", shf);
			c = ord(*wp++);
			if (quotelevel == 0 || c == ORD('"') ||
			    c == ORD('\\') || ctype(c, C_DOLAR | C_GRAVE))
				shf_putc('\\', shf);
			uprntc(c, shf);
			goto closeandout;
		case COMASUB:
			shf_puts("COMASUB<", shf);
			goto dumpsub;
		case COMSUB:
			shf_puts("COMSUB<", shf);
 dumpsub:
			wp = uprntmbs(wp, Nee, shf) + 1;
 closeandout:
			shf_putc('>', shf);
			break;
		case FUNASUB:
			shf_puts("FUNASUB<", shf);
			goto dumpsub;
		case FUNSUB:
			shf_puts("FUNSUB<", shf);
			goto dumpsub;
		case VALSUB:
			shf_puts("VALSUB<", shf);
			goto dumpsub;
		case EXPRSUB:
			shf_puts("EXPRSUB<", shf);
			goto dumpsub;
		case OQUOTE:
			shf_fprintf(shf, "OQUOTE{%d" /*}*/, ++quotelevel);
			break;
		case CQUOTE:
			shf_fprintf(shf, /*{*/ "%d}CQUOTE", quotelevel);
			if (quotelevel)
				quotelevel--;
			else
				shf_puts("(err)", shf);
			break;
		case OSUBST:
			shf_puts("OSUBST(", shf);
			uprntc(*wp++, shf);
			shf_puts(")[", shf);
			wp = uprntmbs(wp, Nee, shf) + 1;
			shf_putc('|', shf);
			wp = dumpwdvar_i(shf, wp, 0);
			break;
		case CSUBST:
			shf_puts("]CSUBST(", shf);
			uprntc(*wp++, shf);
			shf_putc(')', shf);
			return (wp);
		case OPAT:
			shf_puts("OPAT=", shf);
			uprntc(*wp++, shf);
			break;
		case SPAT:
			shf_puts("SPAT", shf);
			break;
		case CPAT:
			shf_puts("CPAT", shf);
			break;
		default:
			shf_fprintf(shf, "INVAL<%u>", (kby)wp[-1]);
			break;
		}
		shf_putc(' ', shf);
	}
}
void
dumpwdvar(struct shf *shf, const char *wp)
{
	dumpwdvar_i(shf, wp, 0);
}

void
dumpioact(struct shf *shf, struct op *t)
{
	struct ioword **ioact, *iop;

	if ((ioact = t->ioact) == NULL)
		return;

	shf_puts("{IOACT", shf);
	while ((iop = *ioact++) != NULL) {
		unsigned short type = iop->ioflag & IOTYPE;
#define DT(x) case x: shf_puts(#x, shf); break;
#define DB(x) if (iop->ioflag & x) shf_puts("|" #x, shf);

		shf_putc(';', shf);
		switch (type) {
		DT(IOREAD)
		DT(IOWRITE)
		DT(IORDWR)
		DT(IOHERE)
		DT(IOCAT)
		DT(IODUP)
		default:
			shf_fprintf(shf, "unk%d", type);
		}
		DB(IOEVAL)
		DB(IOSKIP)
		DB(IOCLOB)
		DB(IORDUP)
		DB(IONAMEXP)
		DB(IOBASH)
		DB(IOHERESTR)
		DB(IONDELIM)
		shf_fprintf(shf, ",unit=%d", (int)iop->unit);
		if (iop->delim && !(iop->ioflag & IONDELIM)) {
			shf_puts(",delim<", shf);
			dumpwdvar(shf, iop->delim);
			shf_putc('>', shf);
		}
		if (iop->ioname) {
			if (iop->ioflag & IONAMEXP) {
				shf_puts(",name=", shf);
				print_value_quoted(shf, iop->ioname);
			} else {
				shf_puts(",name<", shf);
				dumpwdvar(shf, iop->ioname);
				shf_putc('>', shf);
			}
		}
		if (iop->heredoc) {
			shf_puts(",heredoc=", shf);
			print_value_quoted(shf, iop->heredoc);
		}
#undef DT
#undef DB
	}
	shf_putc('}', shf);
}

void
dumptree(struct shf *shf, struct op *t)
{
	int i, j;
	const char **w, *name;
	struct op *t1;
	static int nesting;

	for (i = 0; i < nesting; ++i)
		shf_putc('\t', shf);
	++nesting;
	shf_puts("{tree:" /*}*/, shf);
	if (t == NULL) {
		name = Tnil;
		goto out;
	}
	dumpioact(shf, t);
	switch (t->type) {
#define OPEN(x) case x: name = #x; shf_puts(" {" #x ":", shf); /*}*/

	OPEN(TCOM)
		if (t->vars) {
			i = 0;
			w = (const char **)t->vars;
			while (*w) {
				shf_putc('\n', shf);
				for (j = 0; j < nesting; ++j)
					shf_putc('\t', shf);
				shf_fprintf(shf, " var%d<", i++);
				dumpwdvar(shf, *w++);
				shf_putc('>', shf);
			}
		} else
			shf_puts(" #no-vars#", shf);
		if (t->args) {
			i = 0;
			w = t->args;
			while (*w) {
				shf_putc('\n', shf);
				for (j = 0; j < nesting; ++j)
					shf_putc('\t', shf);
				shf_fprintf(shf, " arg%d<", i++);
				dumpwdvar(shf, *w++);
				shf_putc('>', shf);
			}
		} else
			shf_puts(" #no-args#", shf);
		break;
	OPEN(TEXEC)
 dumpleftandout:
		t = t->left;
 dumpandout:
		shf_putc('\n', shf);
		dumptree(shf, t);
		break;
	OPEN(TPAREN)
		goto dumpleftandout;
	OPEN(TPIPE)
 dumpleftmidrightandout:
		shf_putc('\n', shf);
		dumptree(shf, t->left);
/* middumprightandout: (unused) */
		shf_fprintf(shf, " /%s:", name);
 dumprightandout:
		t = t->right;
		goto dumpandout;
	OPEN(TLIST)
		goto dumpleftmidrightandout;
	OPEN(TOR)
		goto dumpleftmidrightandout;
	OPEN(TAND)
		goto dumpleftmidrightandout;
	OPEN(TBANG)
		goto dumprightandout;
	OPEN(TDBRACKET)
		i = 0;
		w = t->args;
		while (*w) {
			shf_putc('\n', shf);
			for (j = 0; j < nesting; ++j)
				shf_putc('\t', shf);
			shf_fprintf(shf, " arg%d<", i++);
			dumpwdvar(shf, *w++);
			shf_putc('>', shf);
		}
		break;
	OPEN(TFOR)
 dumpfor:
		shf_fprintf(shf, " str<%s>", t->str);
		if (t->vars != NULL) {
			i = 0;
			w = (const char **)t->vars;
			while (*w) {
				shf_putc('\n', shf);
				for (j = 0; j < nesting; ++j)
					shf_putc('\t', shf);
				shf_fprintf(shf, " var%d<", i++);
				dumpwdvar(shf, *w++);
				shf_putc('>', shf);
			}
		}
		goto dumpleftandout;
	OPEN(TSELECT)
		goto dumpfor;
	OPEN(TCASE)
		shf_fprintf(shf, " str<%s>", t->str);
		i = 0;
		for (t1 = t->left; t1 != NULL; t1 = t1->right) {
			shf_putc('\n', shf);
			for (j = 0; j < nesting; ++j)
				shf_putc('\t', shf);
			shf_fprintf(shf, " sub%d[(", i);
			w = (const char **)t1->vars;
			while (*w) {
				dumpwdvar(shf, *w);
				if (w[1] != NULL)
					shf_putc('|', shf);
				++w;
			}
			shf_putc(')', shf);
			dumpioact(shf, t);
			shf_putc('\n', shf);
			dumptree(shf, t1->left);
			shf_fprintf(shf, " ;%c/%d]", t1->u.charflag, i++);
		}
		break;
	OPEN(TWHILE)
		goto dumpleftmidrightandout;
	OPEN(TUNTIL)
		goto dumpleftmidrightandout;
	OPEN(TBRACE)
		goto dumpleftandout;
	OPEN(TCOPROC)
		goto dumpleftandout;
	OPEN(TASYNC)
		goto dumpleftandout;
	OPEN(TFUNCT)
		shf_fprintf(shf, " str<%s> ksh<%s>", t->str,
		    t->u.ksh_func ? Ttrue : Tfalse);
		goto dumpleftandout;
	OPEN(TTIME)
		goto dumpleftandout;
	OPEN(TIF)
 dumpif:
		shf_putc('\n', shf);
		dumptree(shf, t->left);
		t = t->right;
		dumpioact(shf, t);
		if (t->left != NULL) {
			shf_puts(" /TTHEN:\n", shf);
			dumptree(shf, t->left);
		}
		if (t->right && t->right->type == TELIF) {
			shf_puts(" /TELIF:", shf);
			t = t->right;
			dumpioact(shf, t);
			goto dumpif;
		}
		if (t->right != NULL) {
			shf_puts(" /TELSE:\n", shf);
			dumptree(shf, t->right);
		}
		break;
	OPEN(TEOF)
 dumpunexpected:
		shf_puts(Tunexpected, shf);
		break;
	OPEN(TELIF)
		goto dumpunexpected;
	OPEN(TPAT)
		goto dumpunexpected;
	default:
		name = "TINVALID";
		shf_fprintf(shf, "{T<%d>:" /*}*/, t->type);
		goto dumpunexpected;

#undef OPEN
	}
 out:
	shf_fprintf(shf, /*{*/ " /%s}\n", name);
	--nesting;
}

void
dumphex(struct shf *shf, const void *buf, size_t len)
{
	const kby *s = buf;
	size_t i = 0;

	if (!len--) {
		shf_puts("00000000  <\n", shf);
		goto out;
	}

 loop:
	if ((i & 0xFU) == 0x0U)
		shf_fprintf(shf, "%08zX  ", i);
	else if ((i & 0xFU) == 0x8U)
		shf_puts("- ", shf);
	shf_fprintf(shf, "%02X%c", ord(s[i]), i == len ? '<' : ' ');
	if (i < len && (i & 0xFU) != 0xFU) {
		++i;
		goto loop;
	}
	while ((i & 0xFU) != 0xFU) {
		++i;
		if ((i & 0xFU) == 0x8U)
			shf_puts("  ", shf);
		shf_puts("   ", shf);
	}
	shf_puts(" |", shf);
	i &= (size_t)~(size_t)0xFU;
 visloop:
	if (i <= len) {
		shf_putc(ctype(s[i], C_PRINT) ? ord(s[i]) : '.', shf);
		++i;
		if ((i & 0xFU) != 0x0U)
			goto visloop;
	}
	shf_puts("|\n", shf);
	if (i <= len)
		goto loop;
 out:
	shf_flush(shf);
}
#endif
