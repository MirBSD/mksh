#!/bin/sh
# $MirBSD: Build.sh,v 1.6 2004/05/25 22:19:30 tg Exp $
#-
# Copyright (c) 2004
#	Thorsten Glaser <x86@ePost.de>
#
# Licensee is hereby permitted to deal in this work without restric-
# tion, including unlimited rights to use, publically perform, modi-
# fy, merge, distribute, sell, give away or sublicence, provided the
# above copyright notices, these terms and the disclaimer are retai-
# ned in all redistributions, or reproduced in accompanying documen-
# tation or other materials provided with binary redistributions.
#
# Licensor hereby provides this work "AS IS" and WITHOUT WARRANTY of
# any kind, expressed or implied, to the maximum extent permitted by
# applicable law, but with the warranty of being written without ma-
# licious intent or gross negligence; in no event shall an author or
# contributor be held liable for any direct, indirect or other dama-
# ge, however caused, arising in any way out of the usage of covered
# work, even if advised of the possibility of such damage.
#-
# Build the more||less portable mirbsdksh on most operating systems.
# Notes for building on various operating systems:
# - on most OSes, you will need a pre-installed bash or ksh to build
#   because the Bourne shell chokes on some statements below.
# - Solaris: SHELL=ksh LDFLAGS=-ldl WEIRD_OS=1 ksh ./Build.sh
# - Interix: SHELL=ksh ksh ./Build.sh (also on GNU and most *BSD)
# - Mac OSX: SHELL=bash WEIRD_OS=1 bash ./Build.sh

SHELL="${SHELL:-/bin/sh}"; export SHELL
CONFIG_SHELL="${SHELL}"; export CONFIG_SHELL
CC="${CC:-gcc}"
CPPFLAGS="$CPPFLAGS -DHAVE_CONFIG_H -I. -DKSH"
COPTS="-O2 -fomit-frame-pointer -fno-strict-aliasing -fno-strength-reduce"
[ -z "$WEIRD_OS" ] && LDFLAGS="${LDFLAGS:--static}"

if test -e strlfun.c; then
	echo "Configuring..."
	$SHELL ./configure
	echo "Generating prerequisites..."
	$SHELL ./siglist.sh "gcc -E $CPPFLAGS" <siglist.in >siglist.out
	$SHELL ./emacs-gen.sh emacs.c >emacs.out
	echo "Building..."
	$CC $COPTS $CFLAGS $CPPFLAGS $LDFLAGS -o ksh.unstripped *.c
	echo "Finalizing..."
	tbl <ksh.1tbl >ksh.1
	nroff -mdoc -Tascii <ksh.1 >ksh.cat1
	cp ksh.unstripped ksh
	if test -z "$WEIRD_OS"; then
		strip -R .note -R .comment -R .rel.dyn -R .sbss \
		    --strip-unneeded --strip-all ksh \
		    || strip ksh || rm -f ksh
	else
		echo "Trying to strip..."
		strip ksh || \
		echo "Remember to strip the ksh binary yourself!"
	fi
	if test -e ksh; then
		size ksh
	else
		size ksh.unstripped
	fi
	echo "done."
	echo ""
	echo "If you want to test mirbsdksh:"
	echo "perl ./tests/th -s ./tests -p ./ksh -C pdksh,sh,ksh,posix,posix-upu"
	echo ""
	echo "generated files: ksh <- ksh.unstripped   ksh.cat1 <- ksh.1"
else
	echo "Your kit isn't complete, please download the"
	echo "mirbsdksh-1.x.cpio.gz distfile, then extract"
	echo "it and try again! Due to the folks of Ulrich"
	echo "Drepper & co. not including strlcpy/strlcat,"
	echo "this is a necessity to circumvent the broken"
	echo "libc imitation of GNU's."
fi
