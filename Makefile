# $MirOS: src/bin/ksh/Makefile,v 1.1.7.1 2005/03/06 15:42:53 tg Exp $
# $OpenBSD: Makefile,v 1.18 2004/02/16 19:07:19 deraadt Exp $

PROG=	ksh
SRCS=	alloc.c c_ksh.c c_sh.c c_test.c c_ulimit.c edit.c emacs.c eval.c \
	exec.c expr.c history.c io.c jobs.c lex.c main.c misc.c missing.c \
	path.c shf.c syn.c table.c trap.c tree.c tty.c var.c vi.c
MAN=	ksh.1 sh.1

CPPFLAGS+=	-DHAVE_CONFIG_H -I. -DMIRBSD_NATIVE
CFLAGS+=	-Wall -Wextra -pedantic
CLEANFILES+=	emacs.out

LINKS=	${BINDIR}/ksh ${BINDIR}/rksh
LINKS+=	${BINDIR}/ksh ${BINDIR}/sh
MLINKS=	ksh.1 rksh.1 ksh.1 ulimit.1

.depend emacs.o: emacs.out

emacs.out: emacs.c
	${SHELL} ${.CURDIR}/emacs-gen.sh ${.CURDIR}/emacs.c >emacs.out

check:
	/usr/bin/perl ${.CURDIR}/tests/th -s ${.CURDIR}/tests \
	    -p ./ksh -C pdksh,sh,ksh,posix,posix-upu

.include <bsd.prog.mk>
