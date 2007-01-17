# $MirOS: src/bin/mksh/Makefile,v 1.23 2007/01/17 21:42:23 tg Exp $

PROG=		mksh
SRCS=		alloc.c edit.c eval.c exec.c expr.c funcs.c histrap.c \
		jobs.c lex.c main.c misc.c shf.c syn.c tree.c var.c
CPPFLAGS+=	-DHAVE_ATTRIBUTE -DHAVE_ATTRIBUTE_BOUNDED -DHAVE_ATTRIBUTE_USED
CPPFLAGS+=	-DHAVE_SYS_PARAM_H -DHAVE_SYS_SIGNAME -DHAVE_SYS_SIGLIST
CPPFLAGS+=	-DHAVE_ARC4RANDOM -DHAVE_ARC4RANDOM_PUSH -DHAVE_SETLOCALE_CTYPE
CPPFLAGS+=	-DHAVE_LANGINFO_CODESET -DHAVE_SETMODE -DHAVE_SETRESUGID
CPPFLAGS+=	-DHAVE_SETGROUPS -DHAVE_STRCASESTR -DHAVE_STRLCPY
CPPFLAGS+=	-DHAVE_MULTI_IDSTRING
CDIAGFLAGS+=	-Wno-cast-qual

LINKS+=		${BINDIR}/${PROG} ${BINDIR}/sh
MLINKS+=	${PROG}.1 sh.1

regress: ${PROG} check.pl check.t
	-rm -rf regress-dir
	mkdir -p regress-dir
	echo export FNORD=666 >regress-dir/.mkshrc
	HOME=$$(readlink -nf regress-dir) perl ${.CURDIR}/check.pl \
	    -s ${.CURDIR}/check.t -v -p ./${PROG} -C pdksh

cleandir: clean-regress

clean-regress:
	-rm -rf regress-dir

.include <bsd.prog.mk>
