#!/bin/sh
# $MirBSD: Build.sh,v 1.2 2004/05/24 16:24:40 tg Exp $
#-
# Copyright (c) 2004
#	Thorsten "mirabile" Glaser <x86@ePost.de>
#
# Subject to these terms, everybody who obtained a copy of this work
# is hereby permitted to deal in the work without restriction inclu-
# ding without limitation the rights to use, distribute, sell, modi-
# fy, publically perform, give away, merge or sublicence it provided
# this notice is kept and the authors and contributors are given due
# credit in derivates or accompanying documents.
#
# This work is provided by its developers (authors and contributors)
# "as is" and without any warranties whatsoever, express or implied,
# to the maximum extent permitted by applicable law; in no event may
# developers be held liable for damage caused, directly or indirect-
# ly, but not by a developer's malice intent, even if advised of the
# possibility of such damage.
#-
# Build the mirbsdksh on GNU and other operating systems.
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
CFLAGS="-O2 -fomit-frame-pointer -fno-strict-aliasing $CFLAGS"
[ -z "$WEIRD_OS" ] && LDFLAGS="${LDFLAGS:--static}"

if [ -e strlfun.c ]; then
	echo "Configuring..."
	$SHELL ./configure
	echo "Generating prerequisites..."
	$SHELL ./siglist.sh "gcc -E $CPPFLAGS" <siglist.in >siglist.out
	$SHELL ./emacs-gen.sh emacs.c >emacs.out
	echo "Building..."
	$CC $CFLAGS $CPPFLAGS $LDFLAGS -o ksh *.c
	echo "Finalizing..."
	tbl <ksh.1tbl | nroff -mandoc -Tascii >ksh.cat1
	if [ -z "$WEIRD_OS" ]; then
		cp ksh ksh.unstripped
		strip -R .note -R .comment -R .rel.dyn -R .sbss \
		    --strip-unneeded --strip-all ksh \
		    || strip ksh || mv ksh.unstripped ksh
		rm -f ksh.unstripped
	else
		echo "Remember to strip the ksh binary!"
	fi
	size ksh
	echo "done."
	echo ""
	echo "If you want to test mirbsdksh:"
	echo "perl ./tests/th -s ./tests -p ./ksh -C pdksh,sh,ksh,posix,posix-upu"
else
	echo "Your kit isn't complete, please download the"
	echo "mirbsdksh-1.x.cpio.gz distfile, then extract"
	echo "it and try again! Due to the folks of Ulrich"
	echo "Drepper & co. not including strlcpy/strlcat,"
	echo "this is a necessity to circumvent the broken"
	echo "libc imitation of GNU."
fi
