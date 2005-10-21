# $MirOS: src/bin/mksh/Makefile,v 1.8 2005/10/21 11:44:25 tg Exp $

PROG=		mksh
SRCS=		alloc.c edit.c eval.c exec.c expr.c funcs.c histrap.c \
		jobs.c lex.c main.c misc.c shf.c syn.c tree.c var.c
CDIAGFLAGS+=	-Wno-cast-qual

check:
	@cd ${.CURDIR} && ${MAKE} regress V=-v

regress: ${PROG} check.pl check.t
	perl ${.CURDIR}/check.pl -s ${.CURDIR}/check.t ${V} \
	    -p ./${PROG} -C pdksh

.include <bsd.prog.mk>
