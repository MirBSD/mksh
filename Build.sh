#!/bin/sh
# $MirOS: src/bin/mksh/Build.sh,v 1.5 2005/05/25 23:44:50 tg Exp $
#-
# Recognised command line parameters and their defaults:
#	CC		gcc
#	CFLAGS		-O2 -fno-strict-aliasing -fno-strength-reduce
#	CPPFLAGS	(empty)
#	LDFLAGS		-static
#	LIBS		(empty)
#	srcdir		(path of script)
#	NROFF		nroff
# Hints (don't take tgem seriously, WFM rather):
#	GNU/Linux	CPPFLAGS='-D_FILE_OFFSET_BITS=64'
#	Mac OSX		LDFLAGS=
# 	Solaris		LDFLAGS=

SHELL="${SHELL:-/bin/sh}"
CC="${CC:-gcc}"
CFLAGS="${CFLAGS--O2 -fno-strict-aliasing -fno-strength-reduce}"
LDFLAGS="${LDFLAGS--static}"
srcdir="${srcdir:-`dirname $0`}"
curdir="`pwd`"
NROFF="${NROFF:-nroff}"
OS="`uname -s || uname`"
export SHELL CC
if [ x"$1" = x"-q" ]; then
	e=:
	q=1
else
	e=echo
	q=0
fi

SRCS="alloc.c edit.c eval.c exec.c expr.c funcs.c histrap.c"
SRCS="$SRCS jobs.c lex.c main.c misc.c shf.c syn.c tree.c var.c"

# Hello Mr Drepper, we all like you too...</sarcasm>
[ x"$OS" = x"Linux" ] && SRCS="$SRCS strlfun.c"

$e Generating prerequisites...
$SHELL $srcdir/gensigs.sh
for hdr in errno signal; do
	h2ph -d . /usr/include/$hdr.h && mv _h2ph_pre.ph $hdr.ph
done
$e Building...
( cd $srcdir; $CC $CFLAGS $CPPFLAGS $LDFLAGS -o $curdir/mksh $SRCS $LIBS )
test -x mksh || exit 1
$e Finalising...
$NROFF -mdoc <$srcdir/mksh.1 >mksh.cat1 || rm -f mksh.cat1
[ $q = 1 ] || size mksh
$e done.
$e
$e Testing mirbsdksh:
$e "$ perl $srcdir/check.pl -s $srcdir/check.t -p $curdir/mksh -C pdksh"
$e
$e Installing mirbsdksh:
$e "# install -c -s -o root -g bin -m 555 mksh /bin/mksh"
$e "# echo /bin/mksh >>/etc/shells"
$e
$e Installing the manual:
if test -s mksh.cat1; then
	$e "# install -c -o root -g bin -m 444 mksh.cat1" \
	    "/usr/share/man/cat1/mksh.0"
	$e or
fi
$e "# install -c -o root -g bin -m 444 mksh.1 /usr/man/man1/mksh.1"
