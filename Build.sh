#!/bin/sh
# $MirOS: src/bin/mksh/Build.sh,v 1.32 2006/07/03 12:17:08 tg Exp $
#-
# This script recognises CC, CFLAGS, CPPFLAGS, LDFLAGS, LIBS and NROFF.

SHELL="${SHELL:-/bin/sh}"
case $SHELL in
*csh*)	SHELL=/bin/sh ;;
esac
CC="${CC:-gcc}"
CFLAGS="${CFLAGS--O2 -fno-strict-aliasing -fno-strength-reduce -Wall}"
export SHELL
srcdir="${srcdir:-`dirname "$0"`}"
curdir="`pwd`"

e=echo
q=0
r=0
x=0

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
	-x)
		x=1
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
	eval "$@"
}

[ $x = 1 ] || LDSTATIC=-static
[ $x = 1 ] || SRCS=
SRCS="alloc.c edit.c eval.c exec.c expr.c funcs.c histrap.c $SRCS"
SRCS="$SRCS jobs.c lex.c main.c misc.c shf.c syn.c tree.c var.c"

[ $x = 1 ] || case "`uname -s 2>/dev/null || uname`" in
Darwin)
	LDSTATIC= # never works
	CPPFLAGS="$CPPFLAGS -D_FILE_OFFSET_BITS=64"
	;;
Interix)
	CPPFLAGS="$CPPFLAGS -D_ALL_SOURCE -DNEED_COMPAT"
	;;
Linux)
	# Hello Mr Drepper, we all like you too...</sarcasm>
	SRCS="$SRCS compat.c strlfun.c"
	CPPFLAGS="$CPPFLAGS -D_POSIX_C_SOURCE=2 -D_BSD_SOURCE"
	CPPFLAGS="$CPPFLAGS -D_FILE_OFFSET_BITS=64 -DNEED_COMPAT"
	LDSTATIC= # glibc dlopens the PAM library with getpwnam at runtime
	;;
SunOS)
	SRCS="$SRCS compat.c"
	CPPFLAGS="$CPPFLAGS -D_BSD_SOURCE -D_POSIX_C_SOURCE=200112L"
	CPPFLAGS="$CPPFLAGS -D__EXTENSIONS__"
	CPPFLAGS="$CPPFLAGS -D_FILE_OFFSET_BITS=64 -DNEED_COMPAT"
	LDSTATIC= # alternatively you need libdl... same suckage as above
	;;
esac

export CC CPPFLAGS
v $SHELL "'$srcdir/gensigs.sh'" || exit 1
(v "cd '$srcdir' && exec $CC $CFLAGS -I'$curdir' $CPPFLAGS" \
    "$LDFLAGS $LDSTATIC -o '$curdir/mksh' $SRCS $LIBS") || exit 1
test -x mksh || exit 1
[ $r = 1 ] || v "${NROFF:-nroff} -mdoc <'$srcdir/mksh.1' >mksh.cat1" \
    || rm -f mksh.cat1
[ $q = 1 ] || v size mksh
echo '#!/bin/sh' >Test.sh
echo "exec perl '$srcdir/check.pl' -s '$srcdir/check.t' -p '$curdir/mksh' -C pdksh \$*" >>Test.sh
chmod 755 Test.sh
$e
$e To test mirbsdksh, execute ./Test.sh
$e
$e Installing mirbsdksh:
$e "# install -c -s -o root -g bin -m 555 mksh /bin/mksh"
$e "# fgrep -qx /bin/mksh /etc/shells || echo /bin/mksh >>/etc/shells"
$e
$e Installing the manual:
if test -s mksh.cat1; then
	$e "# install -c -o root -g bin -m 444 mksh.cat1" \
	    "/usr/share/man/cat1/mksh.0"
	$e or
fi
$e "# install -c -o root -g bin -m 444 mksh.1 /usr/share/man/man1/mksh.1"
