#!/bin/sh
# $MirOS: src/bin/mksh/Build.sh,v 1.48.2.6 2006/08/28 01:49:14 tg Exp $
#-
# Environment: CC, CFLAGS, CPPFLAGS, LDFLAGS, LIBS, NROFF

v()
{
	$e "$*"
	eval "$@"
}

addcppf()
{
	for i
	do
		eval CPPFLAGS=\"\$CPPFLAGS -D$i=\$$i\"
	done
}

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

if test $x = 0; then
	LDSTATIC=-static
	SRCS=
	sigseen=
	NOWARN=-Wno-error
fi
SRCS="$SRCS alloc.c edit.c eval.c exec.c expr.c funcs.c histrap.c"
SRCS="$SRCS jobs.c lex.c main.c misc.c shf.c syn.c tree.c var.c"

test $x = 1 || case `uname -s 2>/dev/null || uname` in
CYGWIN*)
	LDSTATIC=
	SRCS="$SRCS compat.c"
	CPPFLAGS="$CPPFLAGS -DNEED_COMPAT"
	sigseen=:
	;;
Darwin)
	LDSTATIC=
	CPPFLAGS="$CPPFLAGS -D_FILE_OFFSET_BITS=64"
	;;
Interix)
	CPPFLAGS="$CPPFLAGS -D_ALL_SOURCE -DNEED_COMPAT"
	;;
Linux)
	SRCS="$SRCS compat.c strlfun.c"
	CPPFLAGS="$CPPFLAGS -D_POSIX_C_SOURCE=2 -D_BSD_SOURCE -D_GNU_SOURCE"
	CPPFLAGS="$CPPFLAGS -D_FILE_OFFSET_BITS=64 -DNEED_COMPAT"
	LDSTATIC=
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
	NOWARN=
	r=1
	;;
SunOS)
	SRCS="$SRCS compat.c"
	CPPFLAGS="$CPPFLAGS -D_BSD_SOURCE -D_POSIX_C_SOURCE=200112L"
	CPPFLAGS="$CPPFLAGS -D__EXTENSIONS__"
	CPPFLAGS="$CPPFLAGS -D_FILE_OFFSET_BITS=64 -DNEED_COMPAT"
	LDSTATIC=
	sigseen=:
	r=1
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
		test $nr -gt 0 && test $nr -lt $NSIG || continue
		case $sigseen in
		*:$nr:*) ;;
		*)	echo "		{ $nr, \"$name\" },"
			sigseen=$sigseen$nr:
			;;
		esac
	done >signames.inc
	test -f signames.inc || exit 1
fi

$e Scanning for functions...

test 0 = "$HAVE_ARC4RANDOM" || test 1 = "$HAVE_ARC4RANDOM" ||
{
	$e ... arc4random
	cat >scn.c <<-'EOF'
		#include <stdlib.h>
		int main() { arc4random(); return (0); }
	EOF
	$CC $CFLAGS $CPPFLAGS $LDFLAGS $NOWARN scn.c $LIBS
	if test -f a.out || test -f a.exe; then
		HAVE_ARC4RANDOM=1
		$e "==> arc4random... yes"
	else
		HAVE_ARC4RANDOM=0
		$e "==> arc4random... no"
	fi
	rm -f scn.c a.out a.exe
}

test 0 = "$HAVE_ARC4RANDOM_PUSH" || test 1 = "$HAVE_ARC4RANDOM_PUSH" ||
if test 1 = "$HAVE_ARC4RANDOM"; then
	$e ... arc4random_push
	cat >scn.c <<-'EOF'
		#include <stdlib.h>
		int main() { arc4random_push(1); return (0); }
	EOF
	$CC $CFLAGS $CPPFLAGS $LDFLAGS $NOWARN scn.c $LIBS
	if test -f a.out || test -f a.exe; then
		HAVE_ARC4RANDOM_PUSH=1
		$e "==> arc4random_push... yes"
	else
		HAVE_ARC4RANDOM_PUSH=0
		$e "==> arc4random_push... no"
	fi
	rm -f scn.c a.out a.exe
else
	HAVE_ARC4RANDOM_PUSH=0
fi

$e ... done.
addcppf HAVE_ARC4RANDOM HAVE_ARC4RANDOM_PUSH

(v "cd '$srcdir' && exec $CC $CFLAGS -I'$curdir' $CPPFLAGS" \
    "$LDFLAGS $LDSTATIC -o '$curdir/mksh' $SRCS $LIBS") || exit 1
result=mksh
test -f mksh.exe && result=mksh.exe
test -f $result || exit 1
test $r = 1 || v "$NROFF -mdoc <'$srcdir/mksh.1' >mksh.cat1" || \
    rm -f mksh.cat1
test $q = 1 || v size $result
echo "#!$curdir/mksh" >test.sh
echo "exec perl '$srcdir/check.pl' -s '$srcdir/check.t'" \
    "-p '$curdir/mksh' -C pdksh \$*" >>test.sh
chmod 755 test.sh
i=install
test -f /usr/ucb/$i && i=/usr/ucb/$i
$e
$e Installing the shell:
$e "# $i -c -s -o root -g bin -m 555 mksh /bin/mksh"
$e "# grep -qx /bin/mksh /etc/shells || echo /bin/mksh >>/etc/shells"
$e "# $i -c -o root -g bin -m 444 dot.mkshrc /usr/share/doc/mksh/examples/"
$e
$e Installing the manual:
if test -f mksh.cat1; then
	$e "# $i -c -o root -g bin -m 444 mksh.cat1" \
	    "/usr/share/man/cat1/mksh.0"
	$e or
fi
$e "# $i -c -o root -g bin -m 444 mksh.1 /usr/share/man/man1/mksh.1"
$e
$e Run the regression test suite: ./test.sh
$e Please also read the sample file dot.mkshrc and the fine manual.
