#!/bin/sh
# $MirOS: src/bin/mksh/Build.sh,v 1.1 2005/05/23 16:48:52 tg Exp $

# Recognised command line parameters and their defaults:
#	CC		gcc
#	CFLAGS		-O2 -fno-strict-aliasing -fno-strength-reduce
#	CPPFLAGS	(empty)
#	LDFLAGS		-static
#	LIBS		(empty)
#	srcdir		(path of script)
#	NROFF		nroff

SHELL="${SHELL:-/bin/sh}"
CC="${CC:-gcc}"
CFLAGS="${CFLAGS:--O2 -fno-strict-aliasing -fno-strength-reduce}"
LDFLAGS="${LDFLAGS:--static}"
srcdir="${srcdir:-`dirname $0`}"
curdir="`pwd`"
NROFF="${NROFF:-nroff}"
OS="`uname -s || uname`"
export SHELL CC

SRCS="alloc.c edit.c eval.c exec.c expr.c funcs.c histrap.c"
SRCS="$SRCS jobs.c lex.c main.c misc.c shf.c syn.c tree.c var.c"

[ x"$OS" = x"Linux" ] && SRCS="$SRCS strlfun.c"

echo Generating prerequisites...
$SHELL $srcdir/gensigs.sh
for hdr in errno signal; do
	h2ph -d . /usr/include/$hdr.h && mv _h2ph_pre.ph $hdr.ph
done
echo Building...
#( cd $srcdir; $CC $CFLAGS $CPPFLAGS $LDFLAGS -o $curdir/mksh $SRCS $LIBS )
test -e mksh || exit 1
echo Finalising...
$NROFF -mdoc <$srcdir/mksh.1 >mksh.cat1 || rm -f mksh.cat1
size mksh
echo done.
echo
echo Testing mirbsdksh:
echo "$ perl $srcdir/check.pl -s $srcdir/check.t -p $curdir/mksh -C pdksh"
echo
echo Installing mirbsdksh:
echo "# install -c -s -o root -g bin -m 555 mksh /bin/mksh"
echo "# echo /bin/mksh >>/etc/shells"
echo
echo Installing the manual:
if test -s mksh.cat1; then
	echo "# install -c -o root -g bin -m 444 mksh.cat1" \
	    "/usr/share/man/cat1/mksh.0"
	echo or
fi
echo "# install -c -o root -g bin -m 444 mksh.1 /usr/man/man1/mksh.1"
