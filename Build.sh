#!/bin/sh
# $MirOS: src/bin/mksh/Build.sh,v 1.48.2.5 2006/08/28 01:26:49 tg Exp $
#-
# Environment: CC, CFLAGS, CPPFLAGS, LDFLAGS, LIBS, NROFF

if test -d mksh; then
	echo "$0: Error: ./mksh is a directory!" >&2
	exit 1
fi

: ${CFLAGS='-O2 -fno-strict-aliasing -fno-strength-reduce -Wall'}
: ${CC=gcc} ${NROFF=nroff}
curdir=`pwd` srcdir=`dirname "$0"`
echo | $NROFF -v 2>&1 | grep GNU >&- 2>&- && NROFF="$NROFF -c"

e=echo
q=0
r=0
x=0

for i
do
	case $i in
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
		echo "$0: Unknown option '$i'!" >&2
		exit 1
		;;
	esac
done

v()
{
	$e "$*"
	eval "$@"
}

if test $x = 0; then
	LDSTATIC=-static
	SRCS=
	sigseen=
fi
SRCS="$SRCS alloc.c edit.c eval.c exec.c expr.c funcs.c histrap.c"
SRCS="$SRCS jobs.c lex.c main.c misc.c shf.c syn.c tree.c var.c"

test $x = 1 || case `uname -s 2>/dev/null || uname` in
CYGWIN*)
	LDSTATIC= # they don't want it
	SRCS="$SRCS compat.c"
	CPPFLAGS="$CPPFLAGS -DNEED_COMPAT"
	sigseen=:
	;;
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
	CPPFLAGS="$CPPFLAGS -D_POSIX_C_SOURCE=2 -D_BSD_SOURCE -D_GNU_SOURCE"
	CPPFLAGS="$CPPFLAGS -D_FILE_OFFSET_BITS=64 -DNEED_COMPAT"
	LDSTATIC= # glibc dlopens the PAM library with getpwnam at runtime
	sigseen=:
	;;
Plan9)
	SRCS="$SRCS compat.c strlfun.c"
	CPPFLAGS="$CPPFLAGS -D_POSIX_SOURCE -D_LIMITS_EXTENSION"
	CPPFLAGS="$CPPFLAGS -D_BSD_EXTENSION -D_SUSV2_SOURCE"
	CPPFLAGS="$CPPFLAGS -DNEED_COMPAT -D__Plan9__"
	LDSTATIC=
	CC=cc
	CFLAGS=-O
	r=1
	;;
SunOS)
	SRCS="$SRCS compat.c"
	CPPFLAGS="$CPPFLAGS -D_BSD_SOURCE -D_POSIX_C_SOURCE=200112L"
	CPPFLAGS="$CPPFLAGS -D__EXTENSIONS__"
	CPPFLAGS="$CPPFLAGS -D_FILE_OFFSET_BITS=64 -DNEED_COMPAT"
	LDSTATIC= # alternatively you need libdl... same suckage as above
	sigseen=:
	;;
esac

if test x"$sigseen" = x:; then
	$e Generating list of signal names
	NSIG=`printf %d "\`(echo '#include <signal.h>';echo mksh_cfg: NSIG) | \
	    $CC $CPPFLAGS -E - | grep mksh_cfg | sed 's/^mksh_cfg: //'\`" 2>&-`
	test $NSIG -gt 1 || exit 1
	echo '#include <signal.h>' | $CC $CPPFLAGS -E -dD - | \
	    grep '[	 ]SIG[A-Z0-9]*[	 ]' | \
	    sed 's/^\(.*[	 ]SIG\)\([A-Z0-9]*\)\([	 ].*\)$/\2/' | \
	    while read name; do
		( echo '#include <signal.h>'; echo "mksh_cfg: SIG$name" ) | \
		    $CC $CPPFLAGS -E - | grep mksh_cfg: | \
		    sed 's/^mksh_cfg: \([0-9]*\).*$/\1:'$name/
	done | grep -v '^:' | while IFS=: read nr name; do
		nr=`printf %d "$nr" 2>&-`
		test $nr -gt 0 -a $nr -lt $NSIG || continue
		case $sigseen in
		*:$nr:*) ;;
		*)	echo "		{ $nr, \"$name\" },"
			sigseen=$sigseen$nr:
			;;
		esac
	done >signames.inc
	test -s signames.inc || exit 1
fi
(v "cd '$srcdir' && exec $CC $CFLAGS -I'$curdir' $CPPFLAGS" \
    "$LDFLAGS $LDSTATIC -o '$curdir/mksh' $SRCS $LIBS") || exit 1
test -x mksh || exit 1
test $r = 1 || v "$NROFF -mdoc <'$srcdir/mksh.1' >mksh.cat1" || \
    rm -f mksh.cat1
test $q = 1 || v size mksh
echo "#!$curdir/mksh" >test.sh
echo "exec perl '$srcdir/check.pl' -s '$srcdir/check.t'" \
    "-p '$curdir/mksh' -C pdksh \$*" >>test.sh
chmod 755 test.sh
$e
$e Installing the shell:
$e "# install -c -s -o root -g bin -m 555 mksh /bin/mksh"
$e "# grep -qx /bin/mksh /etc/shells || echo /bin/mksh >>/etc/shells"
$e "# install -c -o root -g bin -m 444 dot.mkshrc /usr/share/doc/mksh/examples/"
$e
$e Installing the manual:
if test -s mksh.cat1; then
	$e "# install -c -o root -g bin -m 444 mksh.cat1" \
	    "/usr/share/man/cat1/mksh.0"
	$e or
fi
$e "# install -c -o root -g bin -m 444 mksh.1 /usr/share/man/man1/mksh.1"
$e
$e Run the regression test suite: ./test.sh
$e Please also read the sample file dot.mkshrc and the manual.
