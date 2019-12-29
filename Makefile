# $MirOS: src/bin/mksh/Makefile,v 1.165 2019/12/29 23:40:40 tg Exp $
#-
# Copyright (c) 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010,
#		2011, 2012, 2013, 2014, 2015, 2016, 2017
#	mirabilos <m@mirbsd.org>
#
# Provided that these terms and disclaimer and all copyright notices
# are retained or reproduced in an accompanying document, permission
# is granted to deal in this work without restriction, including un-
# limited rights to use, publicly perform, distribute, sell, modify,
# merge, give away, or sublicence.
#
# This work is provided "AS IS" and WITHOUT WARRANTY of any kind, to
# the utmost extent permitted by applicable law, neither express nor
# implied; without malicious intent or gross negligence. In no event
# may a licensor, author or contributor be held liable for indirect,
# direct, other damage, loss, or other issues arising in any way out
# of dealing in the work, even if advised of the possibility of such
# damage or existence of a defect, except proven that it results out
# of said person's immediate fault when using the work as intended.

.ifmake d
__CRAZY=	Yes
MKC_DEBG=	cpp
DEBUGFILE=	Yes
NOMAN=		Yes
.endif

.include <bsd.own.mk>

SRCDIR=		${.CURDIR}

PROG=		mksh
SRCS=		edit.c eval.c exec.c expr.c funcs.c histrap.c jobs.c \
		lalloc.c lex.c main.c misc.c shf.c syn.c tree.c var.c
.if !make(test-build)
CPPFLAGS+=	-DMKSH_ASSUME_UTF8 -DMKSH_DISABLE_DEPRECATED \
		-DHAVE_ATTRIBUTE_BOUNDED=1 -DHAVE_ATTRIBUTE_FORMAT=1 \
		-DHAVE_ATTRIBUTE_NORETURN=1 -DHAVE_ATTRIBUTE_PURE=1 \
		-DHAVE_ATTRIBUTE_UNUSED=1 -DHAVE_ATTRIBUTE_USED=1 \
		-DHAVE_SYS_TIME_H=1 -DHAVE_TIME_H=1 -DHAVE_BOTH_TIME_H=1 \
		-DHAVE_SYS_BSDTYPES_H=0 -DHAVE_SYS_FILE_H=1 \
		-DHAVE_SYS_MKDEV_H=0 -DHAVE_SYS_MMAN_H=1 -DHAVE_SYS_PARAM_H=1 \
		-DHAVE_SYS_RESOURCE_H=1 -DHAVE_SYS_SELECT_H=1 \
		-DHAVE_SYS_SYSMACROS_H=0 -DHAVE_BSTRING_H=0 -DHAVE_GRP_H=1 \
		-DHAVE_IO_H=0 -DHAVE_LIBGEN_H=1 -DHAVE_LIBUTIL_H=0 \
		-DHAVE_PATHS_H=1 -DHAVE_STDINT_H=1 -DHAVE_STRINGS_H=1 \
		-DHAVE_TERMIOS_H=1 -DHAVE_ULIMIT_H=0 -DHAVE_VALUES_H=0 \
		-DHAVE_CAN_INTTYPES=1 -DHAVE_CAN_UCBINTS=1 \
		-DHAVE_CAN_INT8TYPE=1 -DHAVE_CAN_UCBINT8=1 -DHAVE_RLIM_T=1 \
		-DHAVE_SIG_T=1 -DHAVE_SYS_ERRLIST=1 -DHAVE_SYS_SIGNAME=1 \
		-DHAVE_SYS_SIGLIST=1 -DHAVE_FLOCK=1 -DHAVE_LOCK_FCNTL=1 \
		-DHAVE_GETRUSAGE=1 -DHAVE_GETSID=1 -DHAVE_GETTIMEOFDAY=1 \
		-DHAVE_KILLPG=1 -DHAVE_MEMMOVE=1 -DHAVE_MKNOD=0 -DHAVE_MMAP=1 \
		-DHAVE_FTRUNCATE=1 -DHAVE_NICE=1 -DHAVE_REVOKE=1 \
		-DHAVE_SETLOCALE_CTYPE=0 -DHAVE_LANGINFO_CODESET=0 \
		-DHAVE_SELECT=1 -DHAVE_SETRESUGID=1 -DHAVE_SETGROUPS=1 \
		-DHAVE_STRERROR=0 -DHAVE_STRSIGNAL=0 -DHAVE_STRLCPY=1 \
		-DHAVE_FLOCK_DECL=1 -DHAVE_REVOKE_DECL=1 \
		-DHAVE_SYS_ERRLIST_DECL=1 -DHAVE_SYS_SIGLIST_DECL=1 \
		-DHAVE_PERSISTENT_HISTORY=1 -DMKSH_BUILD_R=571
CPPFLAGS+=	-D${${PROG:L}_tf:C/(Mir${MAN:E}{0,1}){2}/4/:S/x/mksh_BUILD/:U}
CPPFLAGS+=	-I.
COPTS+=		-std=c89 -Wall
.endif

USE_PRINTF_BUILTIN?=	0
.if ${USE_PRINTF_BUILTIN} == 1
.PATH: ${BSDSRCDIR}/usr.bin/printf
SRCS+=		printf.c
CPPFLAGS+=	-DMKSH_PRINTF_BUILTIN
.endif

DEBUGFILE?=	No
.if ${DEBUGFILE:L} == "yes"
CPPFLAGS+=	-DDF=mksh_debugtofile
.endif

MANLINKS=	[ false pwd sh sleep test true
BINLINKS=	${MANLINKS} echo domainname kill
.for _i in ${BINLINKS}
LINKS+=		${BINDIR}/${PROG} ${BINDIR}/${_i}
.endfor
.for _i in ${MANLINKS}
MLINKS+=	${PROG}.1 ${_i}.1
.endfor

OPTGENS!=	cd ${SRCDIR:Q} && echo *.opt
.for _i in ${OPTGENS}
GENERATED+=	${_i:R}.gen
${_i:R}.gen: ${_i} ${SRCDIR}/Build.sh
	/bin/sh ${SRCDIR:Q}/Build.sh -G ${SRCDIR:Q}/${_i:Q}
.endfor
CLEANFILES+=	${GENERATED}

${PROG} beforedepend: ${GENERATED}

REGRESS_CATEGORIES:=shell:legacy-no,int:32,shell:textmode-no,shell:binmode-yes,fastbox
.if !empty(GCEXTRA:M-DMKSH_FAUX_EBCDIC)
REGRESS_CATEGORIES:=${REGRESS_CATEGORIES},shell:faux-ebcdic
.endif

regress: ${PROG} check.pl check.t
	-rm -rf regress-dir
	mkdir -p regress-dir
	echo export FNORD=666 >regress-dir/.mkshrc
	HOME=$$(realpath regress-dir) perl ${SRCDIR}/check.pl \
	    -s ${SRCDIR}/check.t -v -p ./${PROG} \
	    -C ${REGRESS_CATEGORIES}

TEST_BUILD_ENV:=	TARGET_OS= CPP=
TEST_BUILD_ENV+=	HAVE_STRING_POOLING=0

test-build: .PHONY
	-rm -rf build-dir
	mkdir -p build-dir
.if ${USE_PRINTF_BUILTIN} == 1
	cp ${BSDSRCDIR}/usr.bin/printf/printf.c build-dir/
.endif
	cd build-dir; env CC=${CC:Q} CFLAGS=${CFLAGS:M*:Q} \
	    CPPFLAGS=${CPPFLAGS:M*:Q} LDFLAGS=${LDFLAGS:M*:Q} \
	    LIBS= NOWARN=-Wno-error ${TEST_BUILD_ENV} /bin/sh \
	    ${SRCDIR}/Build.sh -Q -r ${_TBF} && ./test.sh -v -f

CLEANFILES+=	lksh.cat1
test-build-lksh: .PHONY
	cd ${SRCDIR} && exec ${MAKE} lksh.cat1 test-build _TBF=-L

bothmans: .PHONY
	cd ${SRCDIR} && exec ${MAKE} MAN='lksh.1 mksh.1' __MANALL

cleandir: clean-extra

clean-extra: .PHONY
	-rm -rf build-dir regress-dir printf.o printf.ln

mksh_tf=xMakefile${OStype:S/${MACHINE_OS}/1/1g}${OSNAME}
distribution:
	sed 's!\$$I''d\([:$$]\)!$$M''irSecuCron\1!g' \
	    ${SRCDIR}/dot.mkshrc >${DESTDIR}/etc/skel/.mkshrc
	chown ${BINOWN}:${CONFGRP} ${DESTDIR}/etc/skel/.mkshrc
	chmod 0644 ${DESTDIR}/etc/skel/.mkshrc

.if make(cats) || make(clean) || make(cleandir)
MAN=		lksh.1 mksh.1
.endif
CLEANFILES+=	${MANALL:S/.cat/.ps/} ${MAN:S/$/.pdf/} ${MANALL:S/$/.gz/}
CLEANFILES+=	${MAN:S/$/.htm/} ${MAN:S/$/.htm.gz/}
CLEANFILES+=	${MAN:S/$/.txt/} ${MAN:S/$/.txt.gz/}

.include <bsd.prog.mk>

.ifmake cats
V_GROFF!=	pkg_info -e 'groff-*'
V_GHOSTSCRIPT!=	pkg_info -e 'ghostscript-*'
.  if empty(V_GROFF) || empty(V_GHOSTSCRIPT)
.    error empty V_GROFF=${V_GROFF} or V_GHOSTSCRIPT=${V_GHOSTSCRIPT}
.  endif
.endif

CATS_KW=	mksh, lksh, ksh, sh, Korn Shell, Android
CATS_TITLE_lksh_1=lksh - Legacy Korn shell built on mksh
CATS_TITLE_mksh_1=mksh - The MirBSD Korn Shell
cats: ${MANALL} ${MANALL:S/.cat/.ps/}
.if "${MANALL:Nlksh.cat1:Nmksh.cat1}" != ""
.  error Adjust here.
.endif
.for _m _n in lksh 1 mksh 1
	x=$$(ident ${SRCDIR:Q}/${_m}.${_n} | \
	    awk '/Mir''OS:/ { print $$4$$5; }' | \
	    tr -dc 0-9); (( $${#x} == 14 )) || exit 1; exec \
	    ${MKSH} ${BSDSRCDIR:Q}/contrib/hosted/tg/ps2pdfmir -p pa4 -c \
	    -o ${_m}.${_n}.pdf '[' /Author '(The MirOS Project)' \
	    /Title '('${CATS_TITLE_${_m}_${_n}:Q}')' \
	    /Subject '(BSD Reference Manual)' /ModDate "(D:$$x)" \
	    /Creator '(GNU groff version ${V_GROFF:S/groff-//} \(MirPorts\))' \
	    /Producer '(Artifex Ghostscript ${V_GHOSTSCRIPT:S/ghostscript-//:S/-artifex//} \(MirPorts\))' \
	    /Keywords '('${CATS_KW:Q}')' /DOCINFO pdfmark \
	    -f ${_m}.ps${_n}
.endfor
	set -e; . ${BSDSRCDIR:Q}/scripts/roff2htm; set_target_absolute; \
	    for m in ${MANALL}; do \
		bn=$${m%.*}; ext=$${m##*.cat}; \
		[[ $$bn != $$m ]]; [[ $$ext != $$m ]]; \
		gzip -n9 <"$$m" >"$$m.gz"; \
		col -bx <"$$m" >"$$bn.$$ext.txt"; \
		rm -f "$$bn.$$ext.txt.gz"; gzip -n9 "$$bn.$$ext.txt"; \
		do_conversion_verbose "$$bn" "$$ext" "$$m" "$$bn.$$ext.htm"; \
		rm -f "$$bn.$$ext.htm.gz"; gzip -n9 "$$bn.$$ext.htm"; \
	done

.ifmake d
.  ifmake obj || depend || all || install || regress || test-build
d:
.  else
d: all
.  endif
.endif

dr:
	p=$$(realpath ${PROG:Q}) && cd ${SRCDIR:Q} && exec ${MKSH} \
	    ${BSDSRCDIR:Q}/contrib/hosted/tg/sdmksh "$$p"

r:
	cd ${.CURDIR:Q} && exec env ENV=/nonexistent PS1='% ' ${.OBJDIR:Q}/mksh

repool:
	cd ${.CURDIR:Q} && \
	    exec ${MKSH} ${BSDSRCDIR:Q}/scripts/stringpool.sh sh.h

.ifmake validate
.  include "${BSDSRCDIR}/www/Defs.mk"
.endif
validate:
	${_inc2xhtml} <${.CURDIR}/mksh.faq | xmlstarlet val -d ${_xhtmldtd} -e -
	#(cd ${BSDSRCDIR:Q}/www && make obj && exec make validate=mksh-faq)
