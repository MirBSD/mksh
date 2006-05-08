# $MirOS: src/bin/mksh/Makefile,v 1.9 2006/05/08 12:18:49 tg Exp $

PROG=		mksh
SRCS=		alloc.c edit.c eval.c exec.c expr.c funcs.c histrap.c \
		jobs.c lex.c main.c misc.c shf.c syn.c tree.c var.c
CDIAGFLAGS+=	-Wno-cast-qual

check:
	@cd ${.CURDIR} && ${MAKE} regress V=-v

regress: ${PROG} check.pl check.t
	mkdir -p regress-dir
	echo export FNORD=666 >regress-dir/.mkshrc
	HOME=$$(readlink -nf regress-dir) perl ${.CURDIR}/check.pl \
	    -s ${.CURDIR}/check.t ${V} -p ./${PROG} -C pdksh

cleandir: clean-regress

clean-regress:
	-rm -rf regress-dir

.include <bsd.prog.mk>
