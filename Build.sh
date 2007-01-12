#!/bin/sh
# $MirOS: src/bin/mksh/Build.sh,v 1.101 2007/01/12 02:09:10 tg Exp $
#-
# Environment: CC, CFLAGS, CPPFLAGS, LDFLAGS, LIBS, NOWARN, NROFF
# With -x: SRCS (extra), TARGET_OS (uname -s)

v()
{
	$e "$*"
	eval "$@"
}

vq()
{
	eval "$@"
}

if test -t 1; then
	bi=`printf '\033[1m'`
	ui=`printf '\033[4m'`
	ao=`printf '\033[0m'`
else
	bi=
	ui=
	ao=
fi

upper()
{
	echo "$@" | tr qwertyuiopasdfghjklzxcvbnm QWERTYUIOPASDFGHJKLZXCVBNM
}

# pipe .c | ac_test[n] label [!] checkif[!]0 [setlabelifcheckis[!]0] useroutput
ac_testn()
{
	f=$1
	fu=`upper $f`
	fc=0
	if test x"$2" = x""; then
		ft=1
	else
		if test x"$2" = x"!"; then
			fc=1
			shift
		fi
		eval ft=\$HAVE_`upper $2`
		shift
	fi
	fd=$3
	test x"$fd" = x"" && fd=$f
	eval fv=\$HAVE_$fu
	if test 0 = "$fv"; then
		$e "$bi==> $fd...$ao ${ui}no$ao (cached)"
		return
	fi
	if test 1 = "$fv"; then
		$e "$bi==> $fd...$ao ${ui}yes$ao (cached)"
		return
	fi
	if test $fc = "$ft"; then
		fv=$2
		eval HAVE_$fu=$fv
		test 0 = "$fv" && fv=no
		test 1 = "$fv" && fv=yes
		$e "$bi==> $fd...$ao $ui$fv$ao (implied)"
		return
	fi
	$e ... $fd
	cat >scn.c
	v "$CC $CFLAGS $CPPFLAGS $LDFLAGS $NOWARN -I'$srcdir' scn.c $LIBS" \
	    2>&$h | sed 's/^/] /'
	if test -f a.out || test -f a.exe; then
		eval HAVE_$fu=1
		$e "$bi==> $fd...$ao ${ui}yes$ao"
	else
		eval HAVE_$fu=0
		$e "$bi==> $fd...$ao ${ui}no$ao"
	fi
	rm -f scn.c a.out a.exe
}

ac_test()
{
	ac_testn "$@"
	eval CPPFLAGS=\"\$CPPFLAGS -DHAVE_$fu=\$HAVE_$fu\"
}

addsrcs()
{
	eval i=\$$1
	test 0 = $i && case " $SRCS " in
	*\ $2\ *)	;;
	*)		SRCS="$SRCS $2" ;;
	esac
}

if test -d mksh; then
	echo "$0: Error: ./mksh is a directory!" >&2
	exit 1
fi

: ${CFLAGS='-O2 -fno-strict-aliasing -Wall'}
: ${CC=gcc} ${NROFF=nroff}
curdir=`pwd` srcdir=`dirname "$0"` check_categories=pdksh
echo | $NROFF -v 2>&1 | grep GNU >/dev/null 2>&1 && NROFF="$NROFF -c"

e=echo
h=1
r=0
x=0
LDSTATIC=-static

for i
do
	case $i in
	-d)
		LDSTATIC=
		;;
	-q)
		e=:
		h=-
		;;
	-r)
		r=1
		;;
	-x)
		x=1
		LDSTATIC=
		;;
	*)
		echo "$0: Unknown option '$i'!" >&2
		exit 1
		;;
	esac
done

if test $x = 0; then
	SRCS=
	TARGET_OS=`uname -s 2>/dev/null || uname`
fi
SRCS="$SRCS alloc.c edit.c eval.c exec.c expr.c funcs.c histrap.c"
SRCS="$SRCS jobs.c lex.c main.c misc.c shf.c syn.c tree.c var.c"

case $TARGET_OS in
CYGWIN*)
	LDSTATIC=
	;;
Darwin)
	LDSTATIC=
	CPPFLAGS="$CPPFLAGS -D_FILE_OFFSET_BITS=64"
	;;
Interix)
	CPPFLAGS="$CPPFLAGS -D_ALL_SOURCE"
	;;
Linux)
	CPPFLAGS="$CPPFLAGS -D_POSIX_C_SOURCE=2 -D_BSD_SOURCE -D_GNU_SOURCE"
	CPPFLAGS="$CPPFLAGS -D_FILE_OFFSET_BITS=64"
	LDSTATIC=
	;;
SunOS)
	CPPFLAGS="$CPPFLAGS -D_BSD_SOURCE -D__EXTENSIONS__"
	CPPFLAGS="$CPPFLAGS -D_FILE_OFFSET_BITS=64"
	LDSTATIC=
	r=1
	;;
esac

CPPFLAGS="$CPPFLAGS -I'$curdir'"
$e ${ao}Scanning for functions... please ignore any errors.

ac_testn compiler_works '' 'if the compiler works' <<-'EOF'
	int main(void) { return (0); }
EOF
test 1 = $HAVE_COMPILER_WORKS || exit 1

test x"$NOWARN" = x"" && NOWARN=-Wno-error
ac_testn can_wnoerror '' "if '$NOWARN' can be used" <<-'EOF'
	int main(void) { return (0); }
EOF
test 1 = $HAVE_CAN_WNOERROR || NOWARN=

save_NOWARN=$NOWARN
NOWARN=-Werror
ac_testn can_werror '' "if -Werror can be used" <<-'EOF'
	int main(void) { return (0); }
EOF
test 1 = $HAVE_CAN_WERROR || NOWARN=

ac_test attribute '' 'if we have __attribute__((...)) at all' <<-'EOF'
	#include <stdlib.h>
	void fnord(void) __attribute__((noreturn));
	int main(void) { fnord(); }
	void fnord(void) { exit(0); }
EOF

ac_test attribute_bounded attribute 0 'for __attribute__((bounded))' <<-'EOF'
	#include <string.h>
	int xcopy(const void *, void *, size_t)
	    __attribute__((bounded (buffer, 1, 3)))
	    __attribute__((bounded (buffer, 2, 3)));
	int main(int ac, char *av[]) { return (xcopy(av[0], av[1], 1)); }
	int xcopy(const void *s, void *d, size_t n) {
		memmove(d, s, n); return (n);
	}
EOF

ac_test attribute_used attribute 0 'for __attribute__((used))' <<-'EOF'
	static const char fnord[] __attribute__((used)) = "42";
	int main(void) { return (0); }
EOF

NOWARN=$save_NOWARN

ac_testn mksh_full '' "if we're building without MKSH_SMALL" <<-'EOF'
	#ifdef MKSH_SMALL
	#error OK, we are building an extra small mksh.
	#else
	int main(void) { return (0); }
	#endif
EOF

if test 0 = $HAVE_MKSH_FULL; then
	save_CFLAGS=$CFLAGS
	CFLAGS="$CFLAGS -fno-inline"
	ac_testn can_fnoinline '' 'if -fno-inline can be used' <<-'EOF'
		int main(void) { return (0); }
	EOF
	test 1 = $HAVE_CAN_FNOINLINE || CFLAGS=$save_CFLAGS

	HAVE_SETLOCALE_CTYPE=0
	check_categories=$check_categories,smksh
fi

save_CFLAGS=$CFLAGS
CFLAGS="$CFLAGS -fwhole-program --combine"
ac_testn can_fwholepgm '' 'if -fwhole-program --combine can be used' <<-'EOF'
	int main(void) { return (0); }
EOF
test 1 = $HAVE_CAN_FWHOLEPGM || CFLAGS=$save_CFLAGS

ac_test sys_param_h '' '<sys/param.h>' <<'EOF'
	#include <sys/param.h>
	int main(void) { return (0); }
EOF

ac_test mksh_signame '' 'our own list of signal names' <<-'EOF'
	#include <stdlib.h>	/* for NULL */
	#define MKSH_SIGNAMES_CHECK
	#include "signames.c"
	int main(void) { return (mksh_sigpairs[0].nr); }
EOF

ac_test sys_signame '!' mksh_signame 0 'the sys_signame[] array' <<-'EOF'
	extern const char *const sys_signame[];
	int main(void) { return (sys_signame[0][0]); }
EOF

ac_test _sys_signame '!' sys_signame 0 'the _sys_signame[] array' <<-'EOF'
	extern const char *const _sys_signame[];
	int main(void) { return (_sys_signame[0][0]); }
EOF

if test 000 = $HAVE_SYS_SIGNAME$HAVE__SYS_SIGNAME$HAVE_MKSH_SIGNAME; then
	NEED_MKSH_SIGNAME=1
else
	NEED_MKSH_SIGNAME=0
fi

ac_test sys_siglist '' 'the sys_siglist[] array' <<-'EOF'
	extern const char *const sys_siglist[];
	int main(void) { return (sys_siglist[0][0]); }
EOF

ac_test _sys_siglist '!' sys_siglist 0 'the _sys_siglist[] array' <<-'EOF'
	extern const char *const _sys_siglist[];
	int main(void) { return (_sys_siglist[0][0]); }
EOF

ac_test strsignal '!' _sys_siglist 0 <<-'EOF'
	#include <string.h>
	#include <signal.h>
	int main(void) { return (strsignal(1)[0]); }
EOF

ac_test arc4random <<-'EOF'
	#include <stdlib.h>
	int main(void) { return (arc4random()); }
EOF

ac_test arc4random_push arc4random 0 <<-'EOF'
	#include <stdlib.h>
	int main(void) { arc4random_push(1); return (0); }
EOF

ac_test setlocale_ctype '' 'setlocale(LC_CTYPE, "")' <<-'EOF'
	#include <locale.h>
	#include <stddef.h>
	int main(void) { return ((ptrdiff_t)(void *)setlocale(LC_CTYPE, "")); }
EOF

ac_test langinfo_codeset setlocale_ctype 0 'nl_langinfo(CODESET)' <<-'EOF'
	#include <langinfo.h>
	#include <stddef.h>
	int main(void) { return ((ptrdiff_t)(void *)nl_langinfo(CODESET)); }
EOF

ac_test setmode mksh_full 1 <<-'EOF'
	#if defined(__MSVCRT__) || defined(__CYGWIN__)
	#error Win32 setmode() is different from what we need
	#else
	#include <unistd.h>
	int main(int ac, char *av[]) { return (getmode(setmode(av[0]), ac)); }
	#endif
EOF

ac_test setresugid <<-'EOF'
	#include <sys/types.h>
	#include <unistd.h>
	int main(void) { setresuid(0,0,0); return (setresgid(0,0,0)); }
EOF

ac_test setgroups setresugid 0 <<-'EOF'
	#include <sys/types.h>
	#include <grp.h>
	#include <unistd.h>
	int main(void) { gid_t gid = 0; return (setgroups(0, &gid)); }
EOF

ac_test strcasestr <<-'EOF'
	#include <stddef.h>
	#include <string.h>
	int main(int ac, char *av[]) {
		return ((ptrdiff_t)(void *)strcasestr(*av, av[ac]));
	}
EOF

ac_test strlcpy <<-'EOF'
	#include <string.h>
	int main(int ac, char *av[]) { return (strlcpy(*av, av[1], ac)); }
EOF

if test 1 = $NEED_MKSH_SIGNAME; then
	$e "... checking how to run the C Preprocessor"
	rm -f x
	( ( echo '#if (23 * 2 - 2) == (fnord + 2)'
	    echo mksh_rules: fnord
	    echo '#endif'
	  ) | v "$CC -E - $CPPFLAGS -Dfnord=42 >x" ) 2>&$h | sed 's/^/] /'
	if grep '^mksh_rules:.*42' x >/dev/null 2>&1; then
		CPP="$CC -E -"
	else
		rm -f x
		( ( echo '#if (23 * 2 - 2) == (fnord + 2)'
		    echo mksh_rules: fnord
		    echo '#endif'
		  ) | v "$CPP $CPPFLAGS -Dfnord=42 >x" ) 2>&$h | sed 's/^/] /'
		grep '^mksh_rules:.*42' x >/dev/null 2>&1 || CPP=no
	fi
	rm -f x
	$e "$bi==> checking how to run the C Preprocessor...$ao $ui$CPP$ao"
	test x"$CPP" = x"no" && exit 1
fi

$e ... done.

if test 1 = $NEED_MKSH_SIGNAME; then
	$e Generating list of signal names...
	sigseen=:
	NSIG=`( echo '#include <signal.h>'; echo "mksh_cfg: NSIG" ) | \
	    vq "$CPP $CPPFLAGS" | grep mksh_cfg: | \
	    sed 's/^mksh_cfg: \([0-9x]*\).*$/\1/'`
	NSIG=`printf %d "$NSIG" 2>/dev/null`
	test $h = 1 && printf "NSIG=$NSIG ... "
	test $NSIG -gt 1 || exit 1
	echo '#include <signal.h>' | vq "$CPP $CPPFLAGS -dD" | \
	    grep '[	 ]SIG[A-Z0-9]*[	 ]' | \
	    sed 's/^\(.*[	 ]SIG\)\([A-Z0-9]*\)\([	 ].*\)$/\2/' | \
	    while read name; do
		( echo '#include <signal.h>'; echo "mksh_cfg: SIG$name" ) | \
		    vq "$CPP $CPPFLAGS" | grep mksh_cfg: | \
		    sed 's/^mksh_cfg: \([0-9x]*\).*$/\1:'$name/
	done | grep -v '^:' | while IFS=: read nr name; do
		nr=`printf %d "$nr" 2>/dev/null`
		test $nr -gt 0 && test $nr -lt $NSIG || continue
		case $sigseen in
		*:$nr:*) ;;
		*)	echo "		{ $nr, \"$name\" },"
			sigseen=$sigseen$nr:
			test $h = 1 && printf "$nr "
			;;
		esac
	done >signames.inc
	grep ', ' signames.inc >/dev/null 2>&1 || exit 1
	$e done.
fi

addsrcs HAVE_SETMODE setmode.c
addsrcs HAVE_STRCASESTR strcasestr.c
addsrcs HAVE_STRLCPY strlfun.c
CPPFLAGS="$CPPFLAGS -DHAVE_CONFIG_H -DCONFIG_H_FILENAME=\\\"sh.h\\\""

(v "cd '$srcdir' && exec $CC $CFLAGS $CPPFLAGS" \
    "$LDFLAGS $LDSTATIC -o '$curdir/mksh' $SRCS $LIBS") || exit 1
result=mksh
test -f mksh.exe && result=mksh.exe
test -f $result || exit 1
test $r = 1 || v "$NROFF -mdoc <'$srcdir/mksh.1' >mksh.cat1" || \
    rm -f mksh.cat1
test $h = 1 && v size $result
case $curdir in
*\ *)	echo "#!./mksh" >test.sh ;;
*)	echo "#!$curdir/mksh" >test.sh ;;
esac
echo "exec perl '$srcdir/check.pl' -s '$srcdir/check.t'" \
    "-p '$curdir/mksh' -C $check_categories \$*" >>test.sh
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
