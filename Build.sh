#!/bin/sh
# $MirOS: src/bin/mksh/Build.sh,v 1.11 2005/06/08 21:51:20 tg Exp $
#-
# Recognised environment variables and their defaults:
#	CC		gcc
#	CFLAGS		-O2 -fno-strict-aliasing -fno-strength-reduce
#	CPPFLAGS	(empty)
#	LDFLAGS		-static
#	LIBS		(empty)
#	srcdir		(path of script)
#	NROFF		nroff		# (ignored if -r option given)
# Hints (WFM; don't take them seriously):
#	GNU/Linux	CPPFLAGS='-D_FILE_OFFSET_BITS=64'
#	Mac OSX		LDFLAGS=
#	Solaris		LDFLAGS=

SHELL="${SHELL:-/bin/sh}"
CC="${CC:-gcc}"
CFLAGS="${CFLAGS--O2 -fno-strict-aliasing -fno-strength-reduce}"
LDFLAGS="${LDFLAGS--static}"
export SHELL CC
srcdir="${srcdir:-`dirname "$0"`}"
curdir="`pwd`"

if [ x"$1" = x"-q" ]; then
	e=:
	q=1
	shift
else
	e=echo
	q=0
fi
if [ x"$1" = x"-r" ]; then
	r=1
	shift
else
	r=0
fi

v()
{
	$e "$*"
	eval "$*"
}

SRCS="alloc.c edit.c eval.c exec.c expr.c funcs.c histrap.c"
SRCS="$SRCS jobs.c lex.c main.c misc.c shf.c syn.c tree.c var.c"

case "`uname -s 2>/dev/null || uname`" in
Linux)
	# Hello Mr Drepper, we all like you too...</sarcasm>
	SRCS="$SRCS strlfun.c"
	;;
esac

v $SHELL "'$srcdir/gensigs.sh'" || exit 1
(v "cd '$srcdir' && exec $CC $CFLAGS -I'$curdir' $CPPFLAGS $LDFLAGS" \
    "-o '$curdir/mksh' $SRCS $LIBS") || exit 1
test -x mksh || exit 1
v "${NROFF:-nroff} -mdoc <'$srcdir/mksh.1' >mksh.cat1" || rm -f mksh.cat1
[ $q = 1 ] || v size mksh
$e
$e Testing mirbsdksh:
$e "$ perl '$srcdir/check.pl' -s '$srcdir/check.t' -p '$curdir/mksh' -C pdksh"
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
$e "# install -c -o root -g bin -m 444 mksh.1 /usr/share/man/man1/mksh.1"
