#!/bin/sh
# $MirOS: src/bin/mksh/Build.sh,v 1.182.2.3 2007/07/05 11:49:12 tg Exp $
#-
# Environment used: CC CFLAGS CPP CPPFLAGS LDFLAGS LIBS NOWARN NROFF TARGET_OS
# CPPFLAGS recognised: MKSH_SMALL MKSH_ASSUME_UTF8 MKSH_NEED_MKNOD MKSH_NOPWNAM

v()
{
	$e "$*"
	eval "$@"
}

vq()
{
	eval "$@"
}

if test -d /usr/xpg4/bin/. >/dev/null 2>&1; then
	# Solaris: some of the tools have weird behaviour, use portable ones
	PATH=/usr/xpg4/bin:$PATH
	export PATH
fi

if test -n "${ZSH_VERSION+x}" && (emulate sh) >/dev/null 2>&1; then
	emulate sh
	NULLCMD=:
fi

if test -t 1; then
	bi='[1m'
	ui='[4m'
	ao='[0m'
else
	bi=
	ui=
	ao=
fi

allu=QWERTYUIOPASDFGHJKLZXCVBNM
alll=qwertyuiopasdfghjklzxcvbnm
alln=0123456789
alls=______________________________________________________________
nl='
'
tcfn=no
me=`basename "$0"`

upper()
{
	echo :"$@" | sed 's/^://' | tr $alll $allu
}

# pipe .c | ac_test[n] [!] label [!] checkif[!]0 [setlabelifcheckis[!]0] useroutput
ac_testn()
{
	if test x"$1" = x"!"; then
		reverse=1
		shift
	else
		reverse=0
	fi
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
	eval 'v "$CC $CFLAGS $CPPFLAGS $LDFLAGS $NOWARN -I'\''$srcdir'\' \
	    'scn.c $LIBS" 2>&'$h | sed 's/^/] /'
	test x"$tcfn" = x"no" && test -f a.out && tcfn=a.out
	test x"$tcfn" = x"no" && test -f a.exe && tcfn=a.exe
	fr=0
	if test -f $tcfn; then
		test $reverse = 1 || fr=1
	else
		test $reverse = 0 || fr=1
	fi
	if test $fr = 1; then
		eval HAVE_$fu=1
		$e "$bi==> $fd...$ao ${ui}yes$ao"
	else
		eval HAVE_$fu=0
		$e "$bi==> $fd...$ao ${ui}no$ao"
	fi
	rm -f scn.c $tcfn
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
	hv=`echo "$hf" | tr -d '\012\015' | tr -c $alll$allu$alln $alls`
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
	echo "$me: Error: ./mksh is a directory!" >&2
	exit 1
fi
rm -f a.exe a.out *core crypt.exp lft mksh mksh.cat1 mksh.exe no scn.c \
    signames.inc test.sh x

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
		echo "$me: Unknown option '$i'!" >&2
		exit 1
		;;
	esac
done

SRCS="alloc.c edit.c eval.c exec.c expr.c funcs.c histrap.c"
SRCS="$SRCS jobs.c lex.c main.c misc.c shf.c syn.c tree.c var.c"

test $r = 0 && echo | $NROFF -v 2>&1 | grep GNU >/dev/null 2>&1 && \
    NROFF="$NROFF -c"


test x"$TARGET_OS" = x"" && TARGET_OS=`uname -s 2>/dev/null || uname`
warn=
case $TARGET_OS in
AIX)
	warn=' and is still experimental'
	if test x"$LDFLAGS" = x""; then
		LDFLAGS="-Wl,-bI:crypt.exp"
		cat >crypt.exp <<-EOF
			#!
			__crypt_r
			__encrypt_r
			__setkey_r
		EOF
	fi
	: ${LIBS=-lcrypt}
	;;
CYGWIN*)
	test def = $s && s=pam
	;;
Darwin)
	test def = $s && s=pam
	;;
DragonFly)
	;;
FreeBSD)
	;;
GNU)
	CPPFLAGS="$CPPFLAGS -D_GNU_SOURCE"
	;;
GNU/kFreeBSD)
	CPPFLAGS="$CPPFLAGS -D_GNU_SOURCE"
	;;
HP-UX)
	warn=' and is still experimental'
	test def = $s && s=dyn
	case `uname -m` in
	ia64) : ${CFLAGS='-O2 -mlp64'} ;;
	esac
	;;
Interix)
	CPPFLAGS="$CPPFLAGS -D_ALL_SOURCE"
	: ${LIBS=-lcrypt}
	;;
Linux)
	CPPFLAGS="$CPPFLAGS -D_GNU_SOURCE"
	test def = $s && s=pam
	: ${HAVE_REVOKE=0}
	;;
Minix)
	CPPFLAGS="$CPPFLAGS -D_MINIX -D_POSIX_SOURCE"
	warn=' and will currently not work'
#	warn=" but might work with the GNU tools"
#	warn="$warn$nl but not with ACK - /usr/bin/cc - yet)"
	;;
MirBSD)
	;;
NetBSD)
	;;
OpenBSD)
	;;
SunOS)
	CPPFLAGS="$CPPFLAGS -D_BSD_SOURCE -D__EXTENSIONS__"
	test def = $s && s=pam
	r=1
	;;
*)
	warn='; it may or may not work'
	;;
esac

if test -n "$warn"; then
	echo "Warning: mksh has not yet been ported to or tested on your" >&2
	echo "operating system '$TARGET_OS'$warn. If you can provide" >&2
	echo "a shell account to the developer, this may improve; please" >&2
	echo "drop us a success or failure notice or even send in diffs." >&2
fi

CPPFLAGS="$CPPFLAGS -I'$curdir'"


#
# Begin of mirtoconf checks
#
$e $bi$me: Scanning for functions... please ignore any errors.$ao

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
	#undef __attribute__
	void fnord(void) __attribute__((noreturn));
	int main(void) { fnord(); }
	void fnord(void) { exit(0); }
EOF

ac_test attribute_bounded attribute 0 'for __attribute__((bounded))' <<-'EOF'
	#include <string.h>
	#undef __attribute__
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
ac_flags 1 fstackprotectorall "-fstack-protector-all"
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

ac_testn mksh_defutf8 '' "if we assume UTF-8 is enabled" <<-'EOF'
	#ifndef MKSH_ASSUME_UTF8
	#error Nope, we shall check with setlocale() and nl_langinfo(CODESET)
	#endif
	int main(void) { return (0); }
EOF

ac_testn mksh_need_mknod '!' mksh_full 1 'if we still want c_mknod()' <<-'EOF'
	#ifndef MKSH_NEED_MKNOD
	#error Nope, the user really wants it teensy.
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
ac_header sys/mman.h sys/types.h
ac_header sys/sysmacros.h
ac_header libgen.h
ac_header paths.h
ac_header stdbool.h
ac_header stdint.h stdarg.h
ac_header grp.h sys/types.h
ac_header ulimit.h
ac_header values.h

#
# Environment: definitions
#
cat >lft.c <<-'EOF'
	#include <sys/types.h>
	/* check that off_t can represent 2^63-1 correctly, thx FSF */
	#define LARGE_OFF_T (((off_t) 1 << 62) - 1 + ((off_t) 1 << 62))
	int off_t_is_large[(LARGE_OFF_T % 2147483629 == 721 &&
	    LARGE_OFF_T % 2147483647 == 1) ? 1 : -1];
	int main(void) { return (0); }
EOF
ac_testn can_lfs '' "if we support large files" <lft.c
save_CPPFLAGS=$CPPFLAGS
CPPFLAGS="$CPPFLAGS -D_FILE_OFFSET_BITS=64"
ac_testn can_lfs_sus '!' can_lfs 0 "... with -D_FILE_OFFSET_BITS=64" <lft.c
test 1 = $HAVE_CAN_LFS_SUS || CPPFLAGS=$save_CPPFLAGS
save_CPPFLAGS=$CPPFLAGS
CPPFLAGS="$CPPFLAGS -D_LARGE_FILES=1"
ac_testn can_lfs_aix '!' can_lfs 0 "... with -D_LARGE_FILES=1" <lft.c
test 1 = $HAVE_CAN_LFS_AIX || CPPFLAGS=$save_CPPFLAGS
rm -f lft.c	# end of large file support test

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
test x"NetBSD" = x"$TARGET_OS" && $e Ignore the compatibility warning.

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
ac_testn arc4random <<-'EOF'
	#include <sys/types.h>
	#if HAVE_STDINT_H
	#include <stdint.h>
	#endif
	extern u_int32_t arc4random(void);
	int main(void) { return (arc4random()); }
EOF

if test $HAVE_ARC4RANDOM = 0 && test -f "$srcdir/arc4random.c"; then
	ac_header sys/sysctl.h
	addsrcs HAVE_ARC4RANDOM arc4random.c
	HAVE_ARC4RANDOM=1
	HAVE_ARC4RANDOM_DECL=0
	HAVE_ARC4RANDOM_PUSH=0
fi
CPPFLAGS="$CPPFLAGS -DHAVE_ARC4RANDOM=$HAVE_ARC4RANDOM"

ac_test arc4random_push arc4random 0 <<-'EOF'
	extern void arc4random_push(int);
	int main(void) { arc4random_push(1); return (0); }
EOF

ac_test flock_ex '' 'flock and mmap' <<-'EOF'
	#include <sys/types.h>
	#include <sys/file.h>
	#include <sys/mman.h>
	#include <fcntl.h>
	#include <stdlib.h>
	int main(void) { return (mmap(NULL, flock(0, LOCK_EX), PROT_READ,
	    MAP_FILE | MAP_PRIVATE, 0, 0) == NULL ? 1 : 0); }
EOF

ac_test setlocale_ctype '!' mksh_defutf8 0 'setlocale(LC_CTYPE, "")' <<-'EOF'
	#include <locale.h>
	#include <stddef.h>
	int main(void) { return ((ptrdiff_t)(void *)setlocale(LC_CTYPE, "")); }
EOF

ac_test langinfo_codeset setlocale_ctype 0 'nl_langinfo(CODESET)' <<-'EOF'
	#include <langinfo.h>
	#include <stddef.h>
	int main(void) { return ((ptrdiff_t)(void *)nl_langinfo(CODESET)); }
EOF

ac_test revoke mksh_full 0 <<-'EOF'
	#include <unistd.h>
	int main(int ac, char *av[]) { return (ac + revoke(av[0])); }
EOF

ac_test setmode mksh_need_mknod 1 <<-'EOF'
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

ac_test strcasestr setlocale_ctype 1 <<-'EOF'
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
# check headers for declarations
#
ac_test '!' arc4random_decl arc4random 1 'if arc4random() does not need to be declared' <<-'EOF'
	#define MKSH_INCLUDES_ONLY
	#include "sh.h"
	long arc4random(void);		/* this clashes if defined before */
	int main(void) { return (arc4random()); }
EOF
ac_test '!' arc4random_push_decl arc4random_push 1 'if arc4random_push() does not need to be declared' <<-'EOF'
	#define MKSH_INCLUDES_ONLY
	#include "sh.h"
	void arc4random_push(long);	/* this clashes if defined before */
	int main(void) { arc4random_push(1); return (0); }
EOF
ac_test '!' confstr_decl '' 'if confstr() does not need to be declared' <<-'EOF'
	#define MKSH_INCLUDES_ONLY
	#include "sh.h"
	int confstr(long, void *, int);	/* this clashes if defined before */
	int main(int ac, char *av[]) {
	#if !defined(_PATH_DEFPATH) && defined(_CS_PATH)
		return (confstr(ac, *av, 0));
	#else
		return (ac + **av);
	#endif
	}
EOF
ac_test sys_siglist_decl sys_siglist 1 'if sys_siglist[] does not need to be declared' <<-'EOF'
	#define MKSH_INCLUDES_ONLY
	#include "sh.h"
	int main(void) { return (sys_siglist[0][0]); }
EOF

#
# other checks
#
ac_test persistent_history mksh_full 0 'if to use persistent history' <<-'EOF'
	#if !HAVE_FLOCK_EX || defined(MKSH_SMALL)
	#error No, some prerequisites are missing.
	#endif
	int main(void) { return (0); }
EOF
test 1 = $HAVE_PERSISTENT_HISTORY || \
    check_categories=$check_categories,no-histfile

#
# Compiler: Praeprocessor (only if needed)
#
if test 1 = $NEED_MKSH_SIGNAME; then
	$e ... checking how to run the C Preprocessor
	save_CPP=$CPP
	for i in "$save_CPP" "$CC -E -" "cpp" "/usr/libexec/cpp" "/lib/cpp"; do
		CPP=$i
		test x"$CPP" = x"false" && continue
		eval '( ( echo "#if (23 * 2 - 2) == (fnord + 2)"
		    echo mksh_rules: fnord
		    echo "#endif"
		  ) | v "$CPP $CPPFLAGS -Dfnord=42 >x" ) 2>&'$h | \
		    sed 's/^/] /'
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
	NSIG=`( echo '#include <signal.h>'; echo '#ifndef NSIG'; \
	    echo '#define NSIG _NSIG'; echo '#endif'; echo mksh_cfg: NSIG ) | \
	    vq "$CPP $CPPFLAGS" | grep mksh_cfg: | \
	    sed 's/^mksh_cfg:[	 ]*\([0-9x ()+-]*\).*$/\1/'`
	case $NSIG in
	*[\ \(\)+-]*) NSIG=`awk "BEGIN { print $NSIG }"` ;;
	esac
	NSIG=`printf %d "$NSIG" 2>/dev/null`
	test $h = 1 && printf "NSIG=$NSIG ... "
	test $NSIG -gt 1 || exit 1
	echo '#include <signal.h>' | vq "$CPP $CPPFLAGS -dD" | \
	    grep '[	 ]SIG[A-Z0-9]*[	 ]' | \
	    sed 's/^\(.*[	 ]SIG\)\([A-Z0-9]*\)\([	 ].*\)$/\2/' | \
	    sort | while read name; do
		( echo '#include <signal.h>'; echo mksh_cfg: SIG$name ) | \
		    vq "$CPP $CPPFLAGS" | grep mksh_cfg: | \
		    sed 's/^mksh_cfg:[	 ]*\([0-9x]*\).*$/\1:'$name/
	done | grep -v '^:' | while IFS=: read nr name; do
		nr=`printf %d "$nr" 2>/dev/null`
		test $nr -gt 0 && test $nr -le $NSIG || continue
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
addsrcs HAVE_STRLCPY strlcpy.c
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
echo "export PATH='$PATH'" >>test.sh
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
