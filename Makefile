# $MirBSD: Makefile,v 1.2 2004/04/07 17:14:11 tg Exp $
# $OpenBSD: Makefile,v 1.16 2004/01/09 17:10:07 brad Exp $

PROG=	ksh
SRCS=	alloc.c c_ksh.c c_sh.c c_test.c c_ulimit.c edit.c emacs.c \
	eval.c exec.c expr.c history.c io.c jobs.c lex.c mail.c \
	main.c misc.c missing.c path.c shf.c syn.c table.c trap.c \
	tree.c tty.c var.c version.c vi.c
MAN=	ksh.1 sh.1

CPPFLAGS+=	-DHAVE_CONFIG_H -I. -DKSH
CFLAGS+=	-Wall -Werror
CLEANFILES+=	siglist.out emacs.out ksh.1 sh.1

LINKS=	${BINDIR}/ksh ${BINDIR}/rksh
LINKS+=	${BINDIR}/ksh ${BINDIR}/sh
MLINKS=	ksh.1 rksh.1 ksh.1 ulimit.1

.depend trap.o: siglist.out
.depend emacs.o: emacs.out

siglist.out: config.h sh.h siglist.in siglist.sh
	/bin/sh ${.CURDIR}/siglist.sh \
	    "${CC} -E ${CPPFLAGS}" <${.CURDIR}/siglist.in >siglist.out

emacs.out: emacs.c
	/bin/sh ${.CURDIR}/emacs-gen.sh ${.CURDIR}/emacs.c >emacs.out

check test:
	/bin/sh ${.CURDIR}/tests/th.sh ${.CURDIR}/tests/th \
	    -s ${.CURDIR}/tests -p ./ksh -C pdksh,sh,ksh,posix,posix-upu

.include <bsd.prog.mk>
