# $MirOS: src/bin/mksh/Makefile,v 1.11.2.1 2006/08/28 01:49:14 tg Exp $

PROG=		mksh
SRCS=		alloc.c edit.c eval.c exec.c expr.c funcs.c histrap.c \
		jobs.c lex.c main.c misc.c shf.c syn.c tree.c var.c
CPPFLAGS+=	-DHAVE_ARC4RANDOM -DHAVE_ARC4RANDOM_PUSH
CDIAGFLAGS+=	-Wno-cast-qual

LINKS+=		${BINDIR}/${PROG} ${BINDIR}/sh
MLINKS+=	${PROG}.1 sh.1

regress: ${PROG} check.pl check.t
	mkdir -p regress-dir
	echo export FNORD=666 >regress-dir/.mkshrc
	HOME=$$(readlink -nf regress-dir) perl ${.CURDIR}/check.pl \
	    -s ${.CURDIR}/check.t -v -p ./${PROG} -C pdksh

cleandir: clean-regress

clean-regress:
	-rm -rf regress-dir

.include <bsd.prog.mk>
