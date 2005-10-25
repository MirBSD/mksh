#!/bin/sh
# $MirOS: src/bin/mksh/Build.sh,v 1.19 2005/10/25 19:46:52 tg Exp $
#-
# This script recognises CC, CFLAGS, CPPFLAGS, LDFLAGS, LIBS and
# NROFF. Add -d for dynamic linkage (on Mac, GNU/Linux and Solaris).

SHELL="${SHELL:-/bin/sh}"
case $SHELL in
*csh*)	SHELL=/bin/sh ;;
esac
CC="${CC:-gcc}"
CFLAGS="${CFLAGS--O2 -fno-strict-aliasing -fno-strength-reduce -Wall -D_FILE_OFFSET_BITS=64}"
export SHELL CC
srcdir="${srcdir:-`dirname "$0"`}"
curdir="`pwd`"

e=echo
q=0
r=0
LDSTATIC=-static

while [ -n "$1" ]; do
	case $1 in
	-d)
		LDSTATIC=
		;;
	-q)
		e=:
		q=1
		;;
	-r)
		r=1
		;;
	*)
		echo "$0: Unknown option '$1'!" >&2
		exit 1
		;;
	esac
	shift
done

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
	SRCS="$SRCS compat.c strlfun.c"
	;;
SunOS)
	SRCS="$SRCS compat.c"
	CFLAGS="$CFLAGS -Wno-char-subscripts"
	;;
esac

v $SHELL "'$srcdir/gensigs.sh'" || exit 1
(v "cd '$srcdir' && exec $CC $CFLAGS -I'$curdir' $CPPFLAGS" -DNEED_COMPAT \
    "$LDFLAGS $LDSTATIC -o '$curdir/mksh' $SRCS $LIBS") || exit 1
test -x mksh || exit 1
[ $r = 1 ] || v "${NROFF:-nroff} -mdoc <'$srcdir/mksh.1' >mksh.cat1" \
    || rm -f mksh.cat1
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
