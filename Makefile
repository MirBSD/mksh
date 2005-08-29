# $MirOS: src/bin/mksh/Makefile,v 1.5 2005/08/29 01:16:22 tg Exp $

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
