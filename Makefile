# $MirBSD: Makefile,v 1.6 2004/07/15 16:21:04 tg Stab $
# $OpenBSD: Makefile,v 1.18 2004/02/16 19:07:19 deraadt Exp $

PROG=	ksh
SRCS=	alloc.c c_ksh.c c_sh.c c_test.c c_ulimit.c edit.c emacs.c \
	eval.c exec.c expr.c history.c io.c jobs.c lex.c mail.c \
	main.c misc.c missing.c path.c rnd.c shf.c syn.c table.c \
	trap.c tree.c tty.c var.c version.c vi.c
MAN=	ksh.1tbl sh.1tbl

CPPFLAGS+=	-DHAVE_CONFIG_H -I. -DKSH
CFLAGS+=	-Wall -Werror
CLEANFILES+=	siglist.out emacs.out

LINKS=	${BINDIR}/ksh ${BINDIR}/rksh
LINKS+=	${BINDIR}/ksh ${BINDIR}/sh
MLINKS=	ksh.1 rksh.1 ksh.1 ulimit.1

.depend trap.o: siglist.out
.depend emacs.o: emacs.out

siglist.out: config.h sh.h siglist.in siglist.sh
	${SHELL} ${.CURDIR}/siglist.sh \
	    "${CC} -E ${CPPFLAGS}" <${.CURDIR}/siglist.in >siglist.out

emacs.out: emacs.c
	${SHELL} ${.CURDIR}/emacs-gen.sh ${.CURDIR}/emacs.c >emacs.out

check test:
	${SHELL} ${.CURDIR}/tests/th.sh ${.CURDIR}/tests/th \
	    -s ${.CURDIR}/tests -p ./ksh -C pdksh,sh,ksh,posix,posix-upu

.include <bsd.prog.mk>
