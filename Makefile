# $MirOS: src/bin/mksh/Makefile,v 1.7 2005/10/20 12:47:49 tg Exp $

PROG=		mksh
SRCS=		alloc.c edit.c eval.c exec.c expr.c funcs.c histrap.c \
		jobs.c lex.c main.c misc.c shf.c syn.c tree.c var.c
CDIAGFLAGS+=	-Wno-cast-qual

.include <bsd.own.mk>

NO_PRINTF_BUILTIN=broken for now

.ifndef NO_PRINTF_BUILTIN
.PATH: ${BSDSRCDIR}/usr.bin/printf
SRCS+=		printf.c
CFLAGS_printf.o=-DBUILTIN
CPPFLAGS+=	-DUSE_PRINTF
.endif

check:
	@cd ${.CURDIR} && ${MAKE} regress V=-v

regress: ${PROG} check.pl check.t
	perl ${.CURDIR}/check.pl -s ${.CURDIR}/check.t ${V} \
	    -p ./${PROG} -C pdksh

.include <bsd.prog.mk>
