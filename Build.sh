#!/bin/sh
# $MirBSD: src/bin/ksh/Build.sh,v 2.1 2004/12/10 18:09:40 tg Exp $
#-
# Copyright (c) 2004
#	Thorsten "mirabile" Glaser <tg@66h.42h.de>
#
# Licensee is hereby permitted to deal in this work without restric-
# tion, including unlimited rights to use, publicly perform, modify,
# merge, distribute, sell, give away or sublicence, provided all co-
# pyright notices above, these terms and the disclaimer are retained
# in all redistributions or reproduced in accompanying documentation
# or other materials provided with binary redistributions.
#
# Licensor hereby provides this work "AS IS" and WITHOUT WARRANTY of
# any kind, expressed or implied, to the maximum extent permitted by
# applicable law, but with the warranty of being written without ma-
# licious intent or gross negligence; in no event shall licensor, an
# author or contributor be held liable for any damage, direct, indi-
# rect or other, however caused, arising in any way out of the usage
# of this work, even if advised of the possibility of such damage.
#-
# Build the more or less portable mksh on most (non-MirBSD) UNIX®ish
# operating systems.
# Notes for building on various operating systems:
# - Solaris: SHELL=ksh LDFLAGS=-ldl WEIRD_OS=1 sh ./Build.sh
# - Interix: SHELL=ksh sh ./Build.sh (also on GNU and most *BSD)
# - Mac OSX: SHELL=bash WEIRD_OS=1 sh ./Build.sh
#
# Explicit note: you _have_ to use a "modern" bourne-compatible pre-
# installed shell to execute the build script, such as the GNU bash,
# PDKSH or any AT&T KSH. Explicit notice to Debian GNU/Some*nix pak-
# kagers: you also have to set SHELL=/path/to/yourshell in the envi-
# ronment of the script, as shown above.
# Shells known to work:
# - mirbsdksh, pdksh 5.2
# - GNU bash 2.05*
# - Solaris /usr/xpg4/bin/sh
# Shells which should work:
# - AT&T ast-ksh (88 and 93)
# Shells known to *not* work:
# - Solaris /bin/sh (bourne non-POSIX)
# - non-bourne (csh, bsh, ...)
# - zsh

SHELL="${SHELL:-/bin/sh}"; export SHELL
CONFIG_SHELL="${SHELL}"; export CONFIG_SHELL
CC="${CC:-gcc}"
CPPFLAGS="$CPPFLAGS -DHAVE_CONFIG_H -I. -DKSH -DMKSH"
COPTS="-O2 -fomit-frame-pointer -fno-strict-aliasing -fno-strength-reduce"
[ -z "$WEIRD_OS" ] && LDFLAGS="${LDFLAGS:--static}"

if test -e strlfun.c; then
	echo "Preparing testsuite..."
	for hdr in errno signal; do
		h2ph -d . /usr/include/$hdr.h && mv _h2ph_pre.ph $hdr.ph
	done
	echo "Configuring..."
	$SHELL ./configure
	echo "Generating prerequisites..."
	$SHELL ./siglist.sh "$CC -E $CPPFLAGS" <siglist.in >siglist.out
	$SHELL ./emacs-gen.sh emacs.c >emacs.out
	echo "Building..."
	$CC $COPTS $CFLAGS $CPPFLAGS $LDFLAGS -o mksh *.c
	test -e mksh || exit 1
	echo "Finalizing..."
	tbl <ksh.1tbl >mksh.1 || cat ksh.1tbl >mksh.1
	nroff -mdoc -Tascii <mksh.1 >mksh.cat1 || rm -f mksh.cat1
	man=mksh.cat1
	test -s $man || man=mksh.1
	test -s $man || man=ksh.1tbl
	size mksh
	echo "done."
	echo ""
	echo "If you want to test mirbsdksh:"
	echo "perl ./tests/th -s ./tests -p ./mksh -C" \
	    "pdksh,sh,ksh,posix,posix-upu"
	echo ""
	echo "generated files: mksh $man"
	echo ""
	echo "Sample Installation Commands:"
	echo "a) installing the executable"
	echo "# install -c -s -o root -g bin -m 555 mksh /bin/mksh"
	echo "# echo /bin/mksh >>/etc/shells"
	echo "b) installing the manual page (system dependent)"
	if test -s mksh.cat1; then
		echo "   - most Unices, BSD"
		echo "# install -c -o root -g bin -m 444 mksh.cat1" \
		    "/usr/local/man/cat1/mksh.0"
	fi
	if test -s mksh.1; then
		echo "   - some Unices, GNU/Linux"
		echo "# install -c -o root -g bin -m 444 mksh.1" \
		    "/usr/share/man/man1/mksh.1"
	fi
	echo "   - the unformatted manual page"
	echo "=> format ksh.1tbl yourself and copy to the appropriate place."
	echo "=> visit http://wiki.mirbsd.de/MirbsdKsh for online manpages."
else
	echo "Your kit isn't complete, please download the"
	echo "mirbsdksh-Rxx.cpio.gz distfile, then extract"
	echo "it and try again! Due to the folks of Ulrich"
	echo "Drepper & co. not including strlcpy/strlcat,"
	echo "this is a necessity to circumvent the broken"
	echo "libc imitation of GNU's."
fi
