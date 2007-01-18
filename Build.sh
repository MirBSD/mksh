#!/bin/sh
# $MirOS: src/bin/mksh/Build.sh,v 1.144 2007/01/18 16:05:04 tg Exp $
#-
# Env: CC, CFLAGS, CPP, CPPFLAGS, LDFLAGS, LIBS, NOWARN, NROFF, TARGET_OS
# CPPFLAGS recognised: MKSH_SMALL MKSH_NOPWNAM

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

allu=QWERTYUIOPASDFGHJKLZXCVBNM
alll=qwertyuiopasdfghjklzxcvbnm
alln=0123456789

upper()
{
	echo :"$@" | sed 's/^://' | tr $alll $allu
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

# ac_flags add varname flags [text]
ac_flags()
{
	fa=$1
	vn=$2
	f=$3
	ft=$4
	test x"$ft" = x"" && ft="if $f can be used"
	save_CFLAGS=$CFLAGS
	CFLAGS="$CFLAGS $f"
	ac_testn can_$vn '' "$ft" <<-'EOF'
		int main(void) { return (0); }
	EOF
	eval fv=\$HAVE_CAN_`upper $vn`
	test 11 = $fa$fv || CFLAGS=$save_CFLAGS
}

# ac_header header [prereq ...]
ac_header()
{
	hf=$1; shift
	hv=`echo "$hf" | tr -d '\r\n' | tr -c $alll$allu$alln- _`
	for i
	do
		echo "#include <$i>" >>x
	done
	echo "#include <$hf>" >>x
	echo 'int main(void) { return (0); }' >>x
	ac_test "$hv" "" "<$hf>" <x
	rm -f x
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

: ${CC=gcc} ${CPP=false} ${NROFF=nroff}
curdir=`pwd` srcdir=`dirname "$0"` check_categories=pdksh

e=echo
h=1
r=0
s=def

for i
do
	case $i in
	-d)
		s=dyn
		;;
	-nd)
		s=sta
		;;
	-q)
		e=:
		h=-
		;;
	-r)
		r=1
		;;
	*)
		echo "$0: Unknown option '$i'!" >&2
		exit 1
		;;
	esac
done

SRCS="alloc.c edit.c eval.c exec.c expr.c funcs.c histrap.c"
SRCS="$SRCS jobs.c lex.c main.c misc.c shf.c syn.c tree.c var.c"

test $r = 0 && echo | $NROFF -v 2>&1 | grep GNU >/dev/null 2>&1 && \
    NROFF="$NROFF -c"


test x"$TARGET_OS" = x"" && TARGET_OS=`uname -s 2>/dev/null || uname`
case $TARGET_OS in
CYGWIN*)
	test def = $s && s=pam
	;;
Darwin)
	CPPFLAGS="$CPPFLAGS -D_FILE_OFFSET_BITS=64"
	test def = $s && s=pam
	;;
DragonFly)
	;;
FreeBSD)
	;;
Interix)
	CPPFLAGS="$CPPFLAGS -D_ALL_SOURCE"
	;;
Linux)
	CPPFLAGS="$CPPFLAGS -D_POSIX_C_SOURCE=2 -D_BSD_SOURCE -D_GNU_SOURCE"
	CPPFLAGS="$CPPFLAGS -D_FILE_OFFSET_BITS=64"
	test def = $s && s=pam
	;;
MirBSD)
	;;
NetBSD)
	;;
OpenBSD)
	;;
SunOS)
	CPPFLAGS="$CPPFLAGS -D_BSD_SOURCE -D__EXTENSIONS__"
	CPPFLAGS="$CPPFLAGS -D_FILE_OFFSET_BITS=64"
	test def = $s && s=pam
	r=1
	;;
*)
	echo Warning: mksh has not yet been tested on your operating >&2
	echo system "'$TARGET_OS';" it may or may not work. Please drop >&2
	echo us a notice if it works or not or send diffs to make it work. >&2
	;;
esac

CPPFLAGS="$CPPFLAGS -I'$curdir'"


#
# Begin of mirtoconf checks
#
rm -f scn.c a.out a.exe x
$e ${ao}Scanning for functions... please ignore any errors.

#
# Compiler: works as-is, with -Wno-error and -Werror
#
test x"$NOWARN" = x"" && NOWARN=-Wno-error
save_NOWARN=$NOWARN
NOWARN=
ac_flags 0 compiler_works '' 'if the compiler works'
test 1 = $HAVE_CAN_COMPILER_WORKS || exit 1
ac_flags 0 wnoerror "$save_NOWARN"
test 1 = $HAVE_CAN_WNOERROR || save_NOWARN=
ac_flags 0 werror "-Werror"

# The following tests are run with -Werror if possible
test 1 = $HAVE_CAN_WERROR && NOWARN=-Werror

#
# Compiler: check for stuff that only generates warnings
#
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
	int main(int ac, char *av[]) { return (xcopy(av[0], av[--ac], 1)); }
	int xcopy(const void *s, void *d, size_t n) {
		memmove(d, s, n); return (n);
	}
EOF

ac_test attribute_used attribute 0 'for __attribute__((used))' <<-'EOF'
	static const char fnord[] __attribute__((used)) = "42";
	int main(void) { return (0); }
EOF

# End of tests run with -Werror
NOWARN=$save_NOWARN

#
# Compiler: extra flags (-O2 -f* -W* etc.)
#
i=`echo :"$CFLAGS" | sed 's/^://' | tr -c -d $alll$allu$alln-`
test x"$i" = x"" && ac_flags 1 otwo "-O2"
ac_flags 1 fnostrictaliasing "-fno-strict-aliasing"
ac_flags 1 fwholepgm "-fwhole-program --combine"
ac_flags 1 fwrapv "-fwrapv"
# I'd use -std=c99 but this wrecks havoc on glibc and cygwin based
# systems (at least) because their system headers are so broken...
ac_flags 1 stdg99 "-std=gnu99" 'if -std=gnu99 (ISO C99) can be used'
ac_flags 1 wall "-Wall"

#
# mksh: flavours (full/small mksh, omit certain stuff)
#
ac_testn mksh_full '' "if we're building without MKSH_SMALL" <<-'EOF'
	#ifdef MKSH_SMALL
	#error OK, we are building an extra small mksh.
	#endif
	int main(void) { return (0); }
EOF

ac_testn mksh_nopam mksh_full 1 'if the user wants to omit getpwnam()' <<-'EOF'
	#ifndef MKSH_NOPWNAM
	#error No, the user wants to pull in getpwnam.
	#endif
	int main(void) { return (0); }
EOF

if test 0 = $HAVE_MKSH_FULL; then
	ac_flags 1 fnoinline "-fno-inline"

	: ${HAVE_SETLOCALE_CTYPE=0}
	check_categories=$check_categories,smksh
fi

#
# Environment: headers
#
ac_header sys/param.h
ac_header sys/mkdev.h
ac_header sys/sysmacros.h
ac_header libgen.h
ac_header paths.h
ac_header stdbool.h
ac_header grp.h sys/types.h
ac_header ulimit.h
ac_header values.h

#
# Environment: types
#
ac_test rlim_t <<-'EOF'
	#include <sys/types.h>
	#include <sys/time.h>
	#include <sys/resource.h>
	#include <unistd.h>
	int main(void) { return ((int)(rlim_t)0); }
EOF

# only testn: added later below
ac_testn sig_t <<-'EOF'
	#include <sys/types.h>
	#include <signal.h>
	int main(void) { return ((int)(sig_t)0); }
EOF

ac_testn sighandler_t '!' sig_t 0 <<-'EOF'
	#include <sys/types.h>
	#include <signal.h>
	int main(void) { return ((int)(sighandler_t)0); }
EOF
if test 1 = $HAVE_SIGHANDLER_T; then
	CPPFLAGS="$CPPFLAGS -Dsig_t=sighandler_t"
	HAVE_SIG_T=1
fi

ac_testn __sighandler_t '!' sig_t 0 <<-'EOF'
	#include <sys/types.h>
	#include <signal.h>
	int main(void) { return ((int)(__sighandler_t)0); }
EOF
if test 1 = $HAVE___SIGHANDLER_T; then
	CPPFLAGS="$CPPFLAGS -Dsig_t=__sighandler_t"
	HAVE_SIG_T=1
fi

CPPFLAGS="$CPPFLAGS -DHAVE_SIG_T=$HAVE_SIG_T"

#
# Environment: signals
#
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

# only testn: added later below
ac_testn sys_siglist '' 'the sys_siglist[] array' <<-'EOF'
	extern const char *const sys_siglist[];
	int main(void) { return (sys_siglist[0][0]); }
EOF

ac_testn _sys_siglist '!' sys_siglist 0 'the _sys_siglist[] array' <<-'EOF'
	extern const char *const _sys_siglist[];
	int main(void) { return (_sys_siglist[0][0]); }
EOF
if test 1 = $HAVE__SYS_SIGLIST; then
	CPPFLAGS="$CPPFLAGS -Dsys_siglist=_sys_siglist"
	HAVE_SYS_SIGLIST=1
fi
CPPFLAGS="$CPPFLAGS -DHAVE_SYS_SIGLIST=$HAVE_SYS_SIGLIST"

ac_test strsignal '!' sys_siglist 0 <<-'EOF'
	#include <string.h>
	#include <signal.h>
	int main(void) { return (strsignal(1)[0]); }
EOF

#
# Environment: library functions
#
ac_test arc4random <<-'EOF'
	#include <stdlib.h>
	int main(void) { return (arc4random()); }
EOF

ac_test arc4random_push arc4random 0 <<-'EOF'
	#include <stdlib.h>
	int main(void) { arc4random_push(1); return (0); }
EOF

ac_test flock_ex '' 'flock and LOCK_EX' <<-'EOF'
	#include <fcntl.h>
	int main(void) { return (flock(0, LOCK_EX)); }
EOF
test 1 = $HAVE_FLOCK_EX || check_categories=$check_categories,no-histfile

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

ac_test revoke <<-'EOF'
	#include <unistd.h>
	int main(int ac, char *av[]) { return (ac + revoke(av[0])); }
EOF

ac_test setmode mksh_full 1 <<-'EOF'
	#if defined(__MSVCRT__) || defined(__CYGWIN__)
	#error Win32 setmode() is different from what we need
	#endif
	#include <unistd.h>
	int main(int ac, char *av[]) { return (getmode(setmode(av[0]), ac)); }
EOF

ac_test setresugid <<-'EOF'
	#include <sys/types.h>
	#include <unistd.h>
	int main(void) { setresuid(0,0,0); return (setresgid(0,0,0)); }
EOF

ac_test setgroups setresugid 0 <<-'EOF'
	#include <sys/types.h>
	#if HAVE_GRP_H
	#include <grp.h>
	#endif
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

#
# other checks
#
ac_test multi_idstring '' 'if we can use __RCSID(x) multiple times' <<-'EOF'
	#include "sh.h"
	__RCSID("one");
	__RCSID("two");
	int main(void) { return (0); }
EOF

#
# Compiler: Praeprocessor (only if needed)
#
if test 1 = $NEED_MKSH_SIGNAME; then
	$e ... checking how to run the C Preprocessor
	save_CPP=$CPP
	for i in "$save_CPP" "$CC -E -" "cpp" "/usr/libexec/cpp" "/lib/cpp"; do
		CPP=$i
		test x"$CPP" = x"false" && continue
		( ( echo '#if (23 * 2 - 2) == (fnord + 2)'
		    echo mksh_rules: fnord
		    echo '#endif'
		  ) | v "$CPP $CPPFLAGS -Dfnord=42 >x" ) 2>&$h | sed 's/^/] /'
		grep '^mksh_rules:.*42' x >/dev/null 2>&1 || CPP=false
		rm -f x
		test x"$CPP" = x"false" || break
	done
	$e "$bi==> checking how to run the C Preprocessor...$ao $ui$CPP$ao"
	test x"$CPP" = x"false" && exit 1
fi

#
# End of mirtoconf checks
#
$e ... done.

# Some operating systems have ancient versions of ed(1) writing
# the character count to standard output; cope for that
echo wq >x
ed x <x 2>/dev/null | grep 3 >/dev/null 2>&1 && \
    check_categories=$check_categories,oldish-ed
rm -f x

if test 1 = $NEED_MKSH_SIGNAME; then
	$e Generating list of signal names...
	sigseen=:
	NSIG=`( echo '#include <signal.h>'; echo mksh_cfg: NSIG ) | \
	    vq "$CPP $CPPFLAGS" | grep mksh_cfg: | \
	    sed 's/^mksh_cfg: \([0-9x]*\).*$/\1/'`
	NSIG=`printf %d "$NSIG" 2>/dev/null`
	test $h = 1 && printf "NSIG=$NSIG ... "
	test $NSIG -gt 1 || exit 1
	echo '#include <signal.h>' | vq "$CPP $CPPFLAGS -dD" | \
	    grep '[	 ]SIG[A-Z0-9]*[	 ]' | \
	    sed 's/^\(.*[	 ]SIG\)\([A-Z0-9]*\)\([	 ].*\)$/\2/' | \
	    while read name; do
		( echo '#include <signal.h>'; echo mksh_cfg: SIG$name ) | \
		    vq "$CPP $CPPFLAGS" | grep mksh_cfg: | \
		    sed 's/^mksh_cfg: \([0-9x]*\).*$/\1:'$name/
	done | grep -v '^:' | while IFS=: read nr name; do
		nr=`printf %d "$nr" 2>/dev/null`
		test $nr -gt 0 && test $nr -lt $NSIG || continue
		case $sigseen in
		*:$nr:*) ;;
		*)	echo "		{ $nr, \"$name\" },"
			sigseen=$sigseen$nr:
			test $h = 1 && printf "$nr " >&2
			;;
		esac
	done 2>&1 >signames.inc
	grep ', ' signames.inc >/dev/null 2>&1 || exit 1
	$e done.
fi

addsrcs HAVE_SETMODE setmode.c
addsrcs HAVE_STRCASESTR strcasestr.c
addsrcs HAVE_STRLCPY strlfun.c
CPPFLAGS="$CPPFLAGS -DHAVE_CONFIG_H -DCONFIG_H_FILENAME=\\\"sh.h\\\""

case $s:$HAVE_MKSH_NOPAM in
def:*|sta:*|pam:1)
	LDFLAGS="$LDFLAGS -static" ;;
esac
(v "cd '$srcdir' && exec $CC $CFLAGS $CPPFLAGS" \
    "$LDFLAGS -o '$curdir/mksh' $SRCS $LIBS") || exit 1
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
$e "# grep -x /bin/mksh /etc/shells >/dev/null || echo /bin/mksh >>/etc/shells"
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
