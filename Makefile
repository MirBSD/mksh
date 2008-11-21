# $MirOS: src/bin/mksh/Makefile,v 1.70.2.3 2008/11/21 08:53:10 tg Exp $
#-
# use CPPFLAGS=-DDEBUG __CRAZY=Yes to check for certain more stuff

NOMAN=		Yes	# temporary

.include <bsd.own.mk>

PROG=		mksh
SRCS=		aalloc.c edit.c eval.c exec.c expr.c funcs.c histrap.c \
		jobs.c lex.c main.c misc.c shf.c syn.c tree.c var.c
.if !make(test-build)
.  if ${DEBUGLIBS:L} == "yes"
CPPFLAGS+=	-DMKSH_AFREE_DEBUG	# MirOS development version
#CPPFLAGS+=	-DAALLOC_TRACK		# MirOS development version
.  endif
CPPFLAGS+=	-DAALLOC_NO_COOKIES	# for now… aalloc cookies are broken
CPPFLAGS+=	-DMKSH_ASSUME_UTF8 \
		-DHAVE_ATTRIBUTE=1 -DHAVE_ATTRIBUTE_BOUNDED=1 \
		-DHAVE_ATTRIBUTE_USED=1 -DHAVE_SYS_PARAM_H=1 \
		-DHAVE_SYS_MKDEV_H=0 -DHAVE_SYS_MMAN_H=1 \
		-DHAVE_SYS_SYSMACROS_H=0 -DHAVE_LIBGEN_H=1 -DHAVE_LIBUTIL_H=0 \
		-DHAVE_PATHS_H=1 -DHAVE_STDBOOL_H=1 -DHAVE_STRINGS_H=1 \
		-DHAVE_GRP_H=1 -DHAVE_ULIMIT_H=0 -DHAVE_VALUES_H=0 \
		-DHAVE_STDINT_H=1 -DHAVE_RLIM_T=1 -DHAVE_SIG_T=1 \
		-DHAVE_SYS_SIGNAME=1 -DHAVE_SYS_SIGLIST=1 -DHAVE_STRSIGNAL=0 \
		-DHAVE_ARC4RANDOM=1 -DHAVE_ARC4RANDOM_PUSHB=1 -DHAVE_MKNOD=1 \
		-DHAVE_MKSTEMP=1 -DHAVE_NICE=1 -DHAVE_REALPATH=1 \
		-DHAVE_REVOKE=1 -DHAVE_SETLOCALE_CTYPE=1 \
		-DHAVE_LANGINFO_CODESET=1 -DHAVE_SETMODE=1 \
		-DHAVE_SETRESUGID=1 -DHAVE_SETGROUPS=1 -DHAVE_STRCASESTR=1 \
		-DHAVE_STRLCPY=1 -DHAVE_ARC4RANDOM_DECL=1 \
		-DHAVE_ARC4RANDOM_PUSHB_DECL=1 -DHAVE_FLOCK_DECL=1 \
		-DHAVE_REVOKE_DECL=1 -DHAVE_SYS_SIGLIST_DECL=1 \
		-DHAVE_PERSISTENT_HISTORY=1
.if ${CC} == "llvm-gcc"
CPPFLAGS+=	-UHAVE_ATTRIBUTE_BOUNDED -DHAVE_ATTRIBUTE_BOUNDED=0
.endif
COPTS+=		-std=gnu99 -Wall
.endif

LINKS+=		${BINDIR}/${PROG} ${BINDIR}/sh
MLINKS+=	${PROG}.1 sh.1

regress: ${PROG} check.pl check.t
	-rm -rf regress-dir
	mkdir -p regress-dir
	echo export FNORD=666 >regress-dir/.mkshrc
	HOME=$$(realpath regress-dir) perl ${.CURDIR}/check.pl \
	    -s ${.CURDIR}/check.t -v -p ./${PROG}

test-build: .PHONY
	-rm -rf build-dir
	mkdir -p build-dir
	cd build-dir; env CC=${CC:Q} CFLAGS=${CFLAGS:M*:Q} \
	    CPPFLAGS=${CPPFLAGS:M*:Q} LDFLAGS=${LDFLAGS:M*:Q} \
	    LIBS= NOWARN=-Wno-error TARGET_OS= CPP= /bin/sh \
	    ${.CURDIR}/Build.sh -Q -r && ./test.sh -v

cleandir: clean-extra

clean-extra: .PHONY
	-rm -rf build-dir regress-dir

distribution:
	${INSTALL} ${INSTALL_COPY} -o ${BINOWN} -g ${CONFGRP} -m 0644 \
	    ${.CURDIR}/dot.mkshrc ${DESTDIR}/etc/skel/.mkshrc

.include <bsd.prog.mk>
