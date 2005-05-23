# $MirOS: src/bin/mksh/Makefile,v 1.2 2005/05/23 11:59:06 tg Exp $

PROG=		mksh
SRCS=		alloc.c edit.c eval.c exec.c expr.c funcs.c histrap.c \
		jobs.c lex.c main.c misc.c shf.c syn.c tree.c var.c
CDIAGFLAGS+=	-Wno-cast-qual

check:
	@cd ${.CURDIR} && ${MAKE} regress V=-v

regress: ${PROG} check.pl check.t
	perl ${.CURDIR}/check.pl -s ${.CURDIR}/check.t ${V} \
	    -p ./${PROG} -C pdksh,sh,ksh,posix,posix-upu

.include <bsd.prog.mk>
