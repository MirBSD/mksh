#!/bin/sh
srcversion='$MirOS: src/bin/mksh/Build.sh,v 1.373.2.1 2008/11/19 21:10:12 tg Exp $'
#-
# Environment used: CC CFLAGS CPPFLAGS LDFLAGS LIBS NOWARN NROFF TARGET_OS
# CPPFLAGS recognised:	MKSH_SMALL MKSH_ASSUME_UTF8 MKSH_NOPWNAM MKSH_NOVI
#			MKSH_CLS_STRING MKSH_AFREE_DEBUG MKSH_BINSHREDUCED

LC_ALL=C
export LC_ALL

v() {
	$e "$*"
	eval "$@"
}

vv() {
	_c=$1
	shift
	$e "\$ $*" 2>&1
	eval "$@" 2>&1 | sed "s^${_c} "
}

vq() {
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

allu=QWERTYUIOPASDFGHJKLZXCVBNM
alll=qwertyuiopasdfghjklzxcvbnm
alln=0123456789
alls=______________________________________________________________
nl='
'
tcfn=no
bi=
ui=
ao=
fx=
me=`basename "$0"`
orig_CFLAGS=$CFLAGS
phase=x
oldish_ed=stdout-ed,no-stderr-ed

if test -t 1; then
	bi='[1m'
	ui='[4m'
	ao='[0m'
fi

upper() {
	echo :"$@" | sed 's/^://' | tr $alll $allu
}

# clean up after ac_testrun()
ac_testdone() {
	eval HAVE_$fu=$fv
	fr=no
	test 0 = $fv || fr=yes
	$e "$bi==> $fd...$ao $ui$fr$ao$fx"
	fx=
}

# ac_cache label: sets f, fu, fv?=0
ac_cache() {
	f=$1
	fu=`upper $f`
	eval fv=\$HAVE_$fu
	case $fv in
	0|1)
		fx=' (cached)'
		return 0
		;;
	esac
	fv=0
	return 1
}

# ac_testinit label [!] checkif[!]0 [setlabelifcheckis[!]0] useroutput
# returns 1 if value was cached/implied, 0 otherwise: call ac_testdone
ac_testinit() {
	if ac_cache $1; then
		test x"$2" = x"!" && shift
		test x"$2" = x"" || shift
		fd=${3-$f}
		ac_testdone
		return 1
	fi
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
	fd=${3-$f}
	if test $fc = "$ft"; then
		fv=$2
		fx=' (implied)'
		ac_testdone
		return 1
	fi
	$e ... $fd
	return 0
}

# pipe .c | ac_test[n] [!] label [!] checkif[!]0 [setlabelifcheckis[!]0] useroutput
ac_testn() {
	if test x"$1" = x"!"; then
		fr=1
		shift
	else
		fr=0
	fi
	ac_testinit "$@" || return
	cat >scn.c
	vv ']' "$CC $CFLAGS $CPPFLAGS $LDFLAGS $NOWARN scn.c $LIBS $ccpr" | \
	    sed 's^\] scn.c:\([0-9]*\):\] mirtoconf(\1):'
	test $tcfn = no && test -f a.out && tcfn=a.out
	test $tcfn = no && test -f a.exe && tcfn=a.exe
	test $tcfn = no && test -f scn && tcfn=scn
	if test -f $tcfn; then
		test 1 = $fr || fv=1
	else
		test 0 = $fr || fv=1
	fi
	test ugcc=$phase$ct && $CC $CFLAGS $CPPFLAGS $LDFLAGS $NOWARN scn.c \
	    $LIBS 2>&1 | grep 'unrecogni[sz]ed' >/dev/null 2>&1 && fv=$fr
	rm -f scn.c scn.o ${tcfn}*
	ac_testdone
}

ac_cppflags() {
	test x"$1" = x"" || fu=$1
	fv=$2
	test x"$2" = x"" && eval fv=\$HAVE_$fu
	CPPFLAGS="$CPPFLAGS -DHAVE_$fu=$fv"
}

ac_test() {
	ac_testn "$@"
	ac_cppflags
}

# ac_flags [-] add varname flags [text]
ac_flags() {
	if test x"$1" = x"-"; then
		shift
		hf=1
	else
		hf=0
	fi
	fa=$1
	vn=$2
	f=$3
	ft=$4
	test x"$ft" = x"" && ft="if $f can be used"
	save_CFLAGS=$CFLAGS
	CFLAGS="$CFLAGS $f"
	if test 1 = $hf; then
		ac_testn can_$vn '' "$ft"
	else
		ac_testn can_$vn '' "$ft" <<-'EOF'
			int main(void) { return (0); }
		EOF
	fi
	eval fv=\$HAVE_CAN_`upper $vn`
	test 11 = $fa$fv || CFLAGS=$save_CFLAGS
}

# ac_header [!] header [prereq ...]
ac_header() {
	if test x"$1" = x"!"; then
		na=1
		shift
	else
		na=0
	fi
	hf=$1; shift
	hv=`echo "$hf" | tr -d '\012\015' | tr -c $alll$allu$alln $alls`
	for i
	do
		echo "#include <$i>" >>x
	done
	echo "#include <$hf>" >>x
	echo 'int main(void) { return (0); }' >>x
	ac_testn "$hv" "" "<$hf>" <x
	rm -f x
	test 1 = $na || ac_cppflags
}

addsrcs() {
	eval i=\$$1
	test 0 = $i && case " $SRCS " in
	*\ $2\ *)	;;
	*)		SRCS="$SRCS $2" ;;
	esac
}


if test -d mksh || test -d mksh.exe; then
	echo "$me: Error: ./mksh is a directory!" >&2
	exit 1
fi
rm -f a.exe* a.out* *core crypt.exp lft mksh mksh.cat1 mksh.exe mksh.s \
    no *.o scn.c signames.inc stdint.h test.sh x

curdir=`pwd` srcdir=`dirname "$0"` check_categories=

e=echo
r=0
eq=0
pm=0
llvm=NO

for i
do
	case $i in
	-j)
		pm=1
		;;
	-llvm)
		llvm=-std-compile-opts
		;;
	-llvm=*)
		llvm=`echo "x$i" | sed 's/^x-llvm=//'`
		;;
	-Q)
		eq=1
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

SRCS="aalloc.c edit.c eval.c exec.c expr.c funcs.c histrap.c"
SRCS="$SRCS jobs.c lex.c main.c misc.c shf.c syn.c tree.c var.c"

if test x"$srcdir" = x"."; then
	CPPFLAGS="-I. $CPPFLAGS"
else
	CPPFLAGS="-I. -I'$srcdir' $CPPFLAGS"
fi

test x"$TARGET_OS" = x"" && TARGET_OS=`uname -s 2>/dev/null || uname`
warn=
ccpc=-Wc,
ccpl=-Wl,
tsts=
ccpr='|| rm -f ${tcfn}*'
case $TARGET_OS in
AIX)
	CPPFLAGS="$CPPFLAGS -D_ALL_SOURCE"
	if test x"$LDFLAGS" = x""; then
		LDFLAGS="${ccpl}-bI:crypt.exp"
		echo '#!
__crypt_r
__encrypt_r
__setkey_r' >crypt.exp
	fi
	: ${LIBS='-lcrypt'}
	: ${HAVE_SETLOCALE_CTYPE=0}
	;;
BSD/OS)
	: ${HAVE_SETLOCALE_CTYPE=0}
	;;
BeOS|Haiku)
	warn=' and will currently not work'
	;;
CYGWIN*)
	: ${HAVE_SETLOCALE_CTYPE=0}
	;;
Darwin)
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
	;;
Interix)
	ccpc='-X '
	ccpl='-Y '
	CPPFLAGS="$CPPFLAGS -D_ALL_SOURCE"
	: ${LIBS='-lcrypt'}
	: ${HAVE_SETLOCALE_CTYPE=0}
	;;
IRIX*)
	: ${HAVE_SETLOCALE_CTYPE=0}
	;;
Linux)
	CPPFLAGS="$CPPFLAGS -D_GNU_SOURCE"
	: ${HAVE_REVOKE=0}
	;;
MidnightBSD)
	;;
Minix)
	CPPFLAGS="$CPPFLAGS -D_POSIX_SOURCE -D_POSIX_1_SOURCE=2"
	warn=' and will currently not work'
#	warn=" but might work with the GNU tools"
#	warn="$warn${nl}but not with ACK - /usr/bin/cc - yet)"
	: ${HAVE_SETLOCALE_CTYPE=0}
	;;
MirBSD)
	;;
NetBSD)
	;;
OpenBSD)
	: ${HAVE_SETLOCALE_CTYPE=0}
	;;
OSF1)
	HAVE_SIG_T=0	# incompatible
	CPPFLAGS="$CPPFLAGS -D_OSF_SOURCE -D_POSIX_C_SOURCE=200112L"
	CPPFLAGS="$CPPFLAGS -D_XOPEN_SOURCE=600 -D_XOPEN_SOURCE_EXTENDED"
	: ${HAVE_SETLOCALE_CTYPE=0}
	;;
Plan9)
	CPPFLAGS="$CPPFLAGS -D_POSIX_SOURCE -D_LIMITS_EXTENSION"
	CPPFLAGS="$CPPFLAGS -D_BSD_EXTENSION -D_SUSV2_SOURCE"
	warn=' and will currently not work'
	CPPFLAGS="$CPPFLAGS -DMKSH_ASSUME_UTF8"
	;;
PW32*)
	HAVE_SIG_T=0	# incompatible
	warn=' and will currently not work'
	# missing: killpg() getrlimit()
	: ${HAVE_SETLOCALE_CTYPE=0}
	;;
QNX)
	oldish_ed=no-stderr-ed		# oldish /bin/ed is broken
	: ${HAVE_SETLOCALE_CTYPE=0}
	;;
SunOS)
	CPPFLAGS="$CPPFLAGS -D_BSD_SOURCE -D__EXTENSIONS__"
	;;
syllable)
	: ${HAVE_FLOCK_EX=0}
	warn=' and will currently not work'
	;;
ULTRIX)
	: ${CC=cc -YPOSIX}
	CPPFLAGS="$CPPFLAGS -Dssize_t=int"
	: ${HAVE_SETLOCALE_CTYPE=0}
	;;
UWIN*)
	ccpc='-Yc,'
	ccpl='-Yl,'
	tsts=" 3<>/dev/tty"
	warn="; it will compile, but the target"
	warn="$warn${nl}platform itself is very flakey/unreliable"
	: ${HAVE_SETLOCALE_CTYPE=0}
	;;
*)
	warn='; it may or may not work'
	;;
esac

case " $CPPFLAGS " in
*\ -DMKSH_ASSUME_UTF8=0\ *)
	;;
*\ -DMKSH_ASSUME_UTF8*)
	: ${HAVE_SETLOCALE_CTYPE=0}
	;;
esac

if test -n "$warn"; then
	echo "Warning: mksh has not yet been ported to or tested on your" >&2
	echo "operating system '$TARGET_OS'$warn. If you can provide" >&2
	echo "a shell account to the developer, this may improve; please" >&2
	echo "drop us a success or failure notice or even send in diffs." >&2
fi

: ${CC=cc} ${NROFF=nroff}
test 0 = $r && echo | $NROFF -v 2>&1 | grep GNU >/dev/null 2>&1 && \
    NROFF="$NROFF -c"

# this aids me in tracing FTBFSen without access to the buildd
dstversion=`sed -n '/define MKSH_VERSION/s/^.*"\(.*\)".*$/\1/p' $srcdir/sh.h`
$e "Hi from$ao $bi$srcversion$ao on:"
case $TARGET_OS in
Darwin)
	vv '|' "hwprefs machine_type os_type os_class >&2"
	vv '|' "uname -a >&2"
	;;
IRIX*)
	vv '|' "uname -a >&2"
	vv '|' "hinv -v >&2"
	;;
OSF1)
	vv '|' "uname -a >&2"
	vv '|' "/usr/sbin/sizer -v >&2"
	;;
*)
	vv '|' "uname -a >&2"
	;;
esac
$e "$bi$me: Building the MirBSD Korn Shell$ao $ui$dstversion$ao"

#
# Begin of mirtoconf checks
#
$e $bi$me: Scanning for functions... please ignore any errors.$ao

#
# Compiler: which one?
#
# notes:
# – ICC defines __GNUC__ too
# – GCC defines __hpux too
# - LLVM+clang defines __GNUC__ too
# - nwcc defines __GNUC__ too
CPP="$CC -E"
$e ... which compiler seems to be used
echo '#if defined(__ICC) || defined(__INTEL_COMPILER)
ct=icc
#elif defined(__xlC__) || defined(__IBMC__)
ct=xlc
#elif defined(__SUNPRO_C)
ct=sunpro
#elif defined(__BORLANDC__)
ct=bcc
#elif defined(__WATCOMC__)
ct=watcom
#elif defined(__MWERKS__)
ct=metrowerks
#elif defined(__HP_cc)
ct=hpcc
#elif defined(__DECC) || (defined(__osf__) && !defined(__GNUC__))
ct=dec
#elif defined(__PGI)
ct=pgi
#elif defined(__DMC__)
ct=dmc
#elif defined(_MSC_VER)
ct=msc
#elif defined(__ADSPBLACKFIN__) || defined(__ADSPTS__) || defined(__ADSP21000__)
ct=adsp
#elif defined(__IAR_SYSTEMS_ICC__)
ct=iar
#elif defined(SDCC)
ct=sdcc
#elif defined(__PCC__)
ct=pcc
#elif defined(__TenDRA__)
ct=tendra
#elif defined(__TINYC__)
ct=tcc
#elif defined(__llvm__) && defined(__clang__)
ct=clang
#elif defined(__NWCC__)
ct=nwcc
#elif defined(__GNUC__)
ct=gcc
#elif defined(_COMPILER_VERSION)
ct=mipspro
#elif defined(__sgi)
ct=mipspro
#elif defined(__hpux) || defined(__hpua)
ct=hpcc
#elif defined(__ultrix)
ct=ucode
#else
ct=unknown
#endif' >scn.c
ct=unknown
vv ']' "$CPP scn.c | grep ct= | tr -d \\\\015 >x"
sed 's/^/[ /' x
eval `cat x`
rm -f x
echo 'int main(void) { return (0); }' >scn.c
case $ct in
adsp)
	echo >&2 'Warning: Analog Devices C++ compiler for Blackfin, TigerSHARC
    and SHARC (21000) DSPs detected. This compiler has not yet
    been tested for compatibility with mksh. Continue at your
    own risk, please report success/failure to the developers.'
	;;
bcc)
	echo >&2 "Warning: Borland C++ Builder detected. This compiler might
    produce broken executables. Continue at your own risk,
    please report success/failure to the developers."
	;;
clang)
	# does not work with current "ccc" compiler driver
	vv '|' "$CC -version"
	# this works, for now
	vv '|' "${CLANG-clang} -version"
	# ensure compiler and linker are in sync unless overridden
	case $CCC_CC:$CCC_LD in
	:*)	;;
	*:)	CCC_LD=$CCC_CC; export CCC_LD ;;
	esac
	;;
dec)
	vv '|' "$CC -V"
	vv '|' "$CC -Wl,-V scn.c"
	;;
dmc)
	echo >&2 "Warning: Digital Mars Compiler detected. When running under"
	echo >&2 "    UWIN, mksh tends to be unstable due to the limitations"
	echo >&2 "    of this platform. Continue at your own risk,"
	echo >&2 "    please report success/failure to the developers."
	;;
gcc)
	vv '|' "$CC -v scn.c"
	vv '|' 'echo `$CC -dumpmachine` gcc`$CC -dumpversion`'
	;;
hpcc)
	vv '|' "$CC -V scn.c"
	;;
iar)
	echo >&2 'Warning: IAR Systems (http://www.iar.com) compiler for embedded
    systems detected. This unsupported compiler has not yet
    been tested for compatibility with mksh. Continue at your
    own risk, please report success/failure to the developers.'
	;;
icc)
	vv '|' "$CC -V"
	;;
metrowerks)
	echo >&2 'Warning: Metrowerks C compiler detected. This has not yet
    been tested for compatibility with mksh. Continue at your
    own risk, please report success/failure to the developers.'
	;;
mipspro)
	vv '|' "$CC -version"
	;;
msc)
	ccpr=		# errorlevels are not reliable
	case $TARGET_OS in
	Interix)
		if [[ -n $C89_COMPILER ]]; then
			C89_COMPILER=`ntpath2posix -c "$C89_COMPILER"`
		else
			C89_COMPILER=CL.EXE
		fi
		if [[ -n $C89_LINKER ]]; then
			C89_LINKER=`ntpath2posix -c "$C89_LINKER"`
		else
			C89_LINKER=LINK.EXE
		fi
		vv '|' "$C89_COMPILER /HELP >&2"
		vv '|' "$C89_LINKER /LINK >&2"
		;;
	esac
	;;
nwcc)
	vv '|' "$CC -version"
	;;
pcc)
	vv '|' "$CC -v"
	;;
pgi)
	echo >&2 'Warning: PGI detected. This unknown compiler has not yet
    been tested for compatibility with mksh. Continue at your
    own risk, please report success/failure to the developers.'
	;;
sdcc)
	echo >&2 'Warning: sdcc (http://sdcc.sourceforge.net), the small devices
    C compiler for embedded systems detected. This has not yet
    been tested for compatibility with mksh. Continue at your
    own risk, please report success/failure to the developers.'
	;;
sunpro)
	vv '|' "$CC -V scn.c"
	;;
tcc)
	vv '|' "$CC -v"
	;;
tendra)
	vv '|' "$CC -V 2>&1 | fgrep -i -e version -e release"
	;;
ucode)
	vv '|' "$CC -V"
	vv '|' "$CC -Wl,-V scn.c"
	;;
watcom)
	echo >&2 'Warning: Watcom C Compiler detected. This compiler has not yet
    been tested for compatibility with mksh. Continue at your
    own risk, please report success/failure to the developers.'
	;;
xlc)
	vv '|' "$CC -qversion=verbose"
	vv '|' "ld -V"
	;;
*)
	ct=unknown
	;;
esac
test x"$llvm" = x"NO" || vv '|' "llc -version"
$e "$bi==> which compiler seems to be used...$ao $ui$ct$ao"
rm -f scn.c scn.o scn a.out* a.exe*

case $TARGET_OS in
HP-UX)
	case $ct:`uname -m` in
	gcc:ia64) : ${CFLAGS='-mlp64'} ;;
	hpcc:ia64) : ${CFLAGS='+DD64'} ;;
	esac
	;;
esac

#
# Compiler: works as-is, with -Wno-error and -Werror
#
save_NOWARN=$NOWARN
NOWARN=
DOWARN=
ac_flags 0 compiler_works '' 'if the compiler works'
test 1 = $HAVE_CAN_COMPILER_WORKS || exit 1
HAVE_COMPILER_KNOWN=0
test $ct = unknown || HAVE_COMPILER_KNOWN=1
ac_testn compiler_fails '' 'if the compiler does not fail correctly' <<-EOF
	int main(void) { return (thiswillneverbedefinedIhope()); }
EOF
if test 1 = $HAVE_COMPILER_FAILS; then
	save_CFLAGS=$CFLAGS
	: ${HAVE_CAN_DELEXE=x}
	if test $ct = dmc; then
		CFLAGS="$CFLAGS ${ccpl}/DELEXECUTABLE"
		ac_testn can_delexe compiler_fails 0 'for the /DELEXECUTABLE linker option' <<-EOF
			int main(void) { return (0); }
		EOF
	elif test $ct = dec; then
		CFLAGS="$CFLAGS ${ccpl}-non_shared"
		ac_testn can_delexe compiler_fails 0 'for the -non_shared linker option' <<-EOF
			int main(void) { return (0); }
		EOF
	else
		exit 1
	fi
	test 1 = $HAVE_CAN_DELEXE || CFLAGS=$save_CFLAGS
	ac_testn compiler_still_fails '' 'if the compiler still does not fail correctly' <<-EOF
	EOF
	test 1 = $HAVE_COMPILER_STILL_FAILS && exit 1
fi
ac_testn couldbe_tcc '!' compiler_known 0 'if this could be tcc' <<-EOF
	#ifdef __TINYC__
	int main(void) { return (0); }
	#else
	/* force a failure: __TINYC__ not defined */
	int main(void) { return (thiswillneverbedefinedIhope()); }
	#endif
EOF
if test 1 = $HAVE_COULDBE_TCC; then
	ct=tcc
	CPP='cpp -D__TINYC__'
fi

if test $ct = sunpro; then
	test x"$save_NOWARN" = x"" && save_NOWARN='-errwarn=%none'
	ac_flags 0 errwarnnone "$save_NOWARN"
	test 1 = $HAVE_CAN_ERRWARNNONE || save_NOWARN=
	ac_flags 0 errwarnall "-errwarn=%all"
	test 1 = $HAVE_CAN_ERRWARNALL && DOWARN="-errwarn=%all"
elif test $ct = hpcc; then
	save_NOWARN=
	DOWARN=+We
elif test $ct = mipspro; then
	save_NOWARN=
	DOWARN="-diag_error 1-10000"
elif test $ct = msc; then
	save_NOWARN="${ccpc}/w"
	DOWARN="${ccpc}/WX"
elif test $ct = dmc; then
	save_NOWARN="${ccpc}-w"
	DOWARN="${ccpc}-wx"
elif test $ct = bcc; then
	save_NOWARN="${ccpc}-w"
	DOWARN="${ccpc}-w!"
elif test $ct = dec; then
	: -msg_* flags not used yet, or is -w2 correct?
elif test $ct = xlc; then
	save_NOWARN=-qflag=i:e
	DOWARN=-qflag=i:i
elif test $ct = tendra; then
	save_NOWARN=-w
elif test $ct = ucode; then
	save_NOWARN=
	DOWARN=-w2
else
	test x"$save_NOWARN" = x"" && save_NOWARN=-Wno-error
	ac_flags 0 wnoerror "$save_NOWARN"
	test 1 = $HAVE_CAN_WNOERROR || save_NOWARN=
	ac_flags 0 werror -Werror
	test 1 = $HAVE_CAN_WERROR && DOWARN=-Werror
fi

test $ct = icc && DOWARN="$DOWARN -wd1419"
NOWARN=$save_NOWARN

#
# Compiler: extra flags (-O2 -f* -W* etc.)
#
i=`echo :"$orig_CFLAGS" | sed 's/^://' | tr -c -d $alll$allu$alln`
# optimisation: only if orig_CFLAGS is empty
test x"$i" = x"" && if test $ct = sunpro; then
	cat >x <<-'EOF'
		int main(void) { return (0); }
		#define __IDSTRING_CONCAT(l,p)	__LINTED__ ## l ## _ ## p
		#define __IDSTRING_EXPAND(l,p)	__IDSTRING_CONCAT(l,p)
		#define pad			void __IDSTRING_EXPAND(__LINE__,x)(void) { }
	EOF
	yes pad | head -n 256 >>x
	ac_flags - 1 otwo -xO2 <x
	rm -f x
elif test $ct = hpcc; then
	ac_flags 1 otwo +O2
elif test $ct = xlc; then
	ac_flags 1 othree "-O3 -qstrict"
	test 1 = $HAVE_CAN_OTHREE || ac_flags 1 otwo -O2
elif test $ct = tcc || test $ct = tendra; then
	: no special optimisation
else
	ac_flags 1 otwo -O2
	test 1 = $HAVE_CAN_OTWO || ac_flags 1 optimise -O
fi
# other flags: just add them if they are supported
i=0
if test $ct = gcc; then
	# The following tests run with -Werror (gcc only) if possible
	NOWARN=$DOWARN; phase=u
	ac_flags 1 fnostrictaliasing -fno-strict-aliasing
	ac_flags 1 fstackprotectorall -fstack-protector-all
	ac_flags 1 fwrapv -fwrapv
	i=1
elif test $ct = icc; then
	ac_flags 1 fnobuiltinsetmode -fno-builtin-setmode
	ac_flags 1 fnostrictaliasing -fno-strict-aliasing
	ac_flags 1 fstacksecuritycheck -fstack-security-check
	i=1
elif test $ct = sunpro; then
	ac_flags 1 v -v
	ac_flags 1 xc99 -xc99 'for support of ISO C99'
elif test $ct = hpcc; then
	ac_flags 1 agcc -Agcc 'for support of GCC extensions'
	ac_flags 1 ac99 -AC99 'for support of ISO C99'
elif test $ct = dec; then
	ac_flags 0 verb -verbose
	ac_flags 1 rodata -readonly_strings
elif test $ct = dmc; then
	ac_flags 1 decl "${ccpc}-r" 'for strict prototype checks'
	ac_flags 1 schk "${ccpc}-s" 'for stack overflow checking'
elif test $ct = bcc; then
	ac_flags 1 strpool "${ccpc}-d" 'if string pooling can be enabled'
elif test $ct = mipspro; then
	ac_flags 1 xc99 -c99 'for support of ISO C99'
	ac_flags 1 fullwarn -fullwarn 'for remark output support'
elif test $ct = msc; then
	ac_flags 1 strpool "${ccpc}/GF" 'if string pooling can be enabled'
	echo 'int main(void) { char test[64] = ""; return (*test); }' >x
	ac_flags - 1 stackon "${ccpc}/GZ" 'if stack checks can be enabled' <x
	ac_flags - 1 stckall "${ccpc}/Ge" 'stack checks for all functions' <x
	ac_flags - 1 secuchk "${ccpc}/GS" 'for compiler security checks' <x
	rm -f x
	ac_flags 1 wall "${ccpc}/Wall" 'to enable all warnings'
	ac_flags 1 wp64 "${ccpc}/Wp64" 'to enable 64-bit warnings'
elif test $ct = xlc; then
	ac_flags 1 x99 -qlanglvl=extc99
	test 1 = $HAVE_CAN_X99 || ac_flags 1 c99 -qlanglvl=stdc99
	ac_flags 1 rodata "-qro -qroconst -qroptr"
	ac_flags 1 rtcheck -qcheck=all
	ac_flags 1 rtchkc -qextchk
	ac_flags 1 wformat "-qformat=all -qformat=nozln"
	#ac_flags 1 wp64 -qwarn64	# too verbose for now
elif test $ct = tendra; then
	ac_flags 0 ysystem -Ysystem
	test 1 = $HAVE_CAN_YSYSTEM && CPPFLAGS="-Ysystem $CPPFLAGS"
	ac_flags 1 extansi -Xa
elif test $ct = tcc; then
	ac_flags 1 boundschk -b
elif test $ct = clang; then
	i=1
elif test $ct = nwcc; then
	ac_flags 1 ssp -stackprotect
fi
# flags common to a subset of compilers (run with -Werror on gcc)
if test 1 = $i; then
	ac_flags 1 stdg99 -std=gnu99 'for support of ISO C99 + GCC extensions'
	test 1 = $HAVE_CAN_STDG99 || \
	    ac_flags 1 stdc99 -std=c99 'for support of ISO C99'
	ac_flags 1 wall -Wall
fi
phase=x

# The following tests run with -Werror or similar (all compilers) if possible
NOWARN=$DOWARN

#
# Compiler: check for stuff that only generates warnings
#
ac_test attribute '' 'for basic __attribute__((...)) support' <<-'EOF'
	#if defined(__GNUC__) && (__GNUC__ < 2)
	/* force a failure: gcc 1.42 has a false positive here */
	int main(void) { return (thiswillneverbedefinedIhope()); }
	#else
	#include <stdlib.h>
	#undef __attribute__
	void fnord(void) __attribute__((noreturn));
	int main(void) { fnord(); }
	void fnord(void) { exit(0); }
	#endif
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
# mksh: flavours (full/small mksh, omit certain stuff)
#
ac_testn mksh_full '' "if a full-featured mksh is requested" <<-'EOF'
	#ifdef MKSH_SMALL
	/* force a failure: we want a small mksh */
	int main(void) { return (thiswillneverbedefinedIhope()); }
	#else
	/* force a success: we want a full mksh */
	int main(void) { return (0); }
	#endif
EOF
ac_testn mksh_reduced mksh_full 0 "if a reduced-feature sh is requested" <<-'EOF'
	#ifdef MKSH_BINSHREDUCED
	/* force a success: we want a reduced mksh-as-bin-sh */
	int main(void) { return (0); }
	#else
	/* force a failure: we want a full mksh, always */
	int main(void) { return (thiswillneverbedefinedIhope()); }
	#endif
EOF

if test 0 = $HAVE_MKSH_FULL; then
	if test $ct = xlc; then
		ac_flags 1 fnoinline -qnoinline
	else
		ac_flags 1 fnoinline -fno-inline
	fi

	: ${HAVE_MKNOD=0}
	check_categories=$check_categories,smksh
fi
if test 1 = $HAVE_MKSH_REDUCED; then
	check_categories=$check_categories,binsh
fi

#
# Environment: headers
#
ac_header sys/param.h
ac_header sys/mkdev.h sys/types.h
ac_header sys/mman.h sys/types.h
ac_header sys/sysmacros.h
ac_header libgen.h
ac_header libutil.h sys/types.h
ac_header paths.h
ac_header stdbool.h
ac_header strings.h
ac_header grp.h sys/types.h
ac_header ulimit.h sys/types.h
ac_header values.h

ac_header '!' stdint.h stdarg.h
ac_testn can_inttypes '!' stdint_h 1 "for standard 32-bit integer types" <<-'EOF'
	#include <sys/types.h>
	int main(int ac, char **av) { return ((uint32_t)*av + (int32_t)ac); }
EOF
ac_testn can_ucbints '!' can_inttypes 1 "for UCB 32-bit integer types" <<-'EOF'
	#include <sys/types.h>
	int main(int ac, char **av) { return ((u_int32_t)*av + (int32_t)ac); }
EOF
case $HAVE_CAN_INTTYPES$HAVE_CAN_UCBINTS in
01)	HAVE_U_INT32_T=1
	echo 'typedef u_int32_t uint32_t;' >>stdint.h ;;
00)	echo 'typedef signed int int32_t;' >>stdint.h
	echo 'typedef unsigned int uint32_t;' >>stdint.h ;;
esac
test -f stdint.h && HAVE_STDINT_H=1
ac_cppflags STDINT_H

#
# Environment: definitions
#
echo '#include <sys/types.h>
/* check that off_t can represent 2^63-1 correctly, thx FSF */
#define LARGE_OFF_T (((off_t) 1 << 62) - 1 + ((off_t) 1 << 62))
int off_t_is_large[(LARGE_OFF_T % 2147483629 == 721 &&
    LARGE_OFF_T % 2147483647 == 1) ? 1 : -1];
int main(void) { return (0); }' >lft.c
ac_testn can_lfs '' "for large file support" <lft.c
save_CPPFLAGS=$CPPFLAGS
CPPFLAGS="$CPPFLAGS -D_FILE_OFFSET_BITS=64"
ac_testn can_lfs_sus '!' can_lfs 0 "... with -D_FILE_OFFSET_BITS=64" <lft.c
if test 0 = $HAVE_CAN_LFS_SUS; then
	CPPFLAGS="$save_CPPFLAGS -D_LARGE_FILES=1"
	ac_testn can_lfs_aix '!' can_lfs 0 "... with -D_LARGE_FILES=1" <lft.c
	test 1 = $HAVE_CAN_LFS_AIX || CPPFLAGS=$save_CPPFLAGS
fi
rm -f lft.c lft.o	# end of large file support test

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
	#include <stddef.h>
	int main(void) { return ((ptrdiff_t)(sig_t)0); }
EOF

ac_testn sighandler_t '!' sig_t 0 <<-'EOF'
	#include <sys/types.h>
	#include <signal.h>
	#include <stddef.h>
	int main(void) { return ((ptrdiff_t)(sighandler_t)0); }
EOF
if test 1 = $HAVE_SIGHANDLER_T; then
	CPPFLAGS="$CPPFLAGS -Dsig_t=sighandler_t"
	HAVE_SIG_T=1
fi

ac_testn __sighandler_t '!' sig_t 0 <<-'EOF'
	#include <sys/types.h>
	#include <signal.h>
	#include <stddef.h>
	int main(void) { return ((ptrdiff_t)(__sighandler_t)0); }
EOF
if test 1 = $HAVE___SIGHANDLER_T; then
	CPPFLAGS="$CPPFLAGS -Dsig_t=__sighandler_t"
	HAVE_SIG_T=1
fi

test 1 = $HAVE_SIG_T || CPPFLAGS="$CPPFLAGS -Dsig_t=nosig_t"
ac_cppflags SIG_T

ac_testn u_int32_t <<-'EOF'
	#include <sys/types.h>
	#if HAVE_STDINT_H
	#include <stdint.h>
	#endif
	int main(void) { return ((int)(u_int32_t)0); }
EOF
test 1 = $HAVE_U_INT32_T || CPPFLAGS="$CPPFLAGS -Du_int32_t=uint32_t"

#
# Environment: signals
#
test x"NetBSD" = x"$TARGET_OS" && $e Ignore the compatibility warning.

for what in name list; do
	uwhat=`upper $what`
	ac_testn sys_sig$what '' "the sys_sig${what}[] array" <<-EOF
		extern const char *const sys_sig${what}[];
		int main(void) { return (sys_sig${what}[0][0]); }
	EOF
	ac_testn _sys_sig$what '!' sys_sig$what 0 "the _sys_sig${what}[] array" <<-EOF
		extern const char *const _sys_sig${what}[];
		int main(void) { return (_sys_sig${what}[0][0]); }
	EOF
	if eval "test 1 = \$HAVE__SYS_SIG$uwhat"; then
		CPPFLAGS="$CPPFLAGS -Dsys_sig$what=_sys_sig$what"
		eval "HAVE_SYS_SIG$uwhat=1"
	fi
	ac_cppflags SYS_SIG$uwhat
done

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

save_LIBS=$LIBS
if test 0 = $HAVE_ARC4RANDOM; then
	test -f arc4random.c || if test -f "$srcdir/arc4random.c"; then
		# ensure isolation of source directory from build directory
		cp "$srcdir/arc4random.c" .
	fi
	if test -f arc4random.c; then
		ac_header sys/sysctl.h
		addsrcs HAVE_ARC4RANDOM arc4random.c
		HAVE_ARC4RANDOM=1
		LIBS="$LIBS arc4random.c"
	fi
fi
ac_cppflags ARC4RANDOM

ac_test arc4random_pushb arc4random 0 <<-'EOF'
	#include <sys/types.h>
	#if HAVE_STDINT_H
	#include <stdint.h>
	#endif
	extern uint32_t arc4random_pushb(void *, size_t);
	int main(int ac, char *av[]) { return (arc4random_pushb(*av, ac)); }
EOF
LIBS=$save_LIBS

ac_testn flock_ex '' 'flock and mmap' <<-'EOF'
	#include <sys/types.h>
	#include <sys/file.h>
	#include <sys/mman.h>
	#include <fcntl.h>
	#include <stdlib.h>
	int main(void) { return ((void *)mmap(NULL, flock(0, LOCK_EX),
	    PROT_READ, MAP_PRIVATE, 0, 0) == (void *)NULL ? 1 : 0); }
EOF

ac_test mknod '' 'if to use mknod(), makedev() and friends' <<-'EOF'
	#define MKSH_INCLUDES_ONLY
	#include "sh.h"
	int main(int ac, char *av[]) {
		dev_t dv;
		dv = makedev(ac, 1);
		return (mknod(av[0], 0, dv) ? (int)major(dv) : (int)minor(dv));
	}
EOF

ac_test mkstemp <<-'EOF'
	#include <stdlib.h>
	int main(void) { char tmpl[] = "X"; return (mkstemp(tmpl)); }
EOF

ac_test nice <<-'EOF'
	#include <unistd.h>
	int main(void) { return (nice(4)); }
EOF

ac_test realpath mksh_full 0 <<-'EOF'
	#if HAVE_SYS_PARAM_H
	#include <sys/param.h>
	#endif
	#include <stdlib.h>
	#ifndef PATH_MAX
	#define PATH_MAX 1024
	#endif
	char *res, dst[PATH_MAX];
	const char src[] = ".";
	int main(void) {
		res = realpath(src, dst);
		return (res == NULL ? 1 : 0);
	}
EOF

ac_test revoke mksh_full 0 <<-'EOF'
	#if HAVE_LIBUTIL_H
	#include <libutil.h>
	#endif
	#include <unistd.h>
	int main(int ac, char *av[]) { return (ac + revoke(av[0])); }
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

ac_test setmode mknod 1 <<-'EOF'
	#if defined(__MSVCRT__) || defined(__CYGWIN__)
	/* force a failure: Win32 setmode() is not what we want… */
	int main(void) { return (thiswillneverbedefinedIhope()); }
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
	#if HAVE_GRP_H
	#include <grp.h>
	#endif
	#include <unistd.h>
	int main(void) { gid_t gid = 0; return (setgroups(0, &gid)); }
EOF

ac_test strcasestr <<-'EOF'
	#include <stddef.h>
	#include <string.h>
	#if HAVE_STRINGS_H
	#include <strings.h>
	#endif
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
save_CC=$CC; save_LDFLAGS=$LDFLAGS; save_LIBS=$LIBS
CC="$CC -c -o $tcfn"; LDFLAGS=; LIBS=
ac_test '!' arc4random_decl arc4random 1 'if arc4random() does not need to be declared' <<-'EOF'
	#define MKSH_INCLUDES_ONLY
	#include "sh.h"
	long arc4random(void);		/* this clashes if defined before */
	int main(void) { return (arc4random()); }
EOF
ac_test '!' arc4random_pushb_decl arc4random_pushb 1 'if arc4random_pushb() does not need to be declared' <<-'EOF'
	#define MKSH_INCLUDES_ONLY
	#include "sh.h"
	int arc4random_pushb(char, int); /* this clashes if defined before */
	int main(int ac, char *av[]) { return (arc4random_pushb(**av, ac)); }
EOF
ac_test '!' flock_decl flock_ex 1 'if flock() does not need to be declared' <<-'EOF'
	#define MKSH_INCLUDES_ONLY
	#include "sh.h"
	long flock(void);		/* this clashes if defined before */
	int main(void) { return (flock()); }
EOF
ac_test '!' revoke_decl revoke 1 'if revoke() does not need to be declared' <<-'EOF'
	#define MKSH_INCLUDES_ONLY
	#include "sh.h"
	long revoke(void);		/* this clashes if defined before */
	int main(void) { return (revoke()); }
EOF
ac_test sys_siglist_decl sys_siglist 1 'if sys_siglist[] does not need to be declared' <<-'EOF'
	#define MKSH_INCLUDES_ONLY
	#include "sh.h"
	int main(void) { return (sys_siglist[0][0]); }
EOF
CC=$save_CC; LDFLAGS=$save_LDFLAGS; LIBS=$save_LIBS

#
# other checks
#
fd='if to use persistent history'
ac_cache PERSISTENT_HISTORY || test 11 != $HAVE_FLOCK_EX$HAVE_MKSH_FULL || fv=1
test 1 = $fv || check_categories=$check_categories,no-histfile
ac_testdone
ac_cppflags

#
# Compiler: Praeprocessor (only if needed)
#
test 0 = $HAVE_SYS_SIGNAME && if ac_testinit cpp_dd '' \
    'checking if the C Preprocessor supports -dD'; then
	echo '#define foo bar' >scn.c
	vv ']' "$CPP -dD scn.c >x"
	grep '#define foo bar' x >/dev/null 2>&1 && fv=1
	rm -f scn.c x
	ac_testdone
fi

#
# End of mirtoconf checks
#
$e ... done.

# Some operating systems have ancient versions of ed(1) writing
# the character count to standard output; cope for that
echo wq >x
ed x <x 2>/dev/null | grep 3 >/dev/null 2>&1 && \
    check_categories=$check_categories,$oldish_ed
rm -f x

if test 0 = $HAVE_SYS_SIGNAME; then
	if test 1 = $HAVE_CPP_DD; then
		$e Generating list of signal names...
	else
		$e No list of signal names available via cpp. Falling back...
	fi
	sigseen=:
	echo '#include <signal.h>
#ifndef NSIG
#if defined(_NSIG)
#define NSIG _NSIG
#elif defined(SIGMAX)
#define NSIG (SIGMAX+1)
#endif
#endif
mksh_cfg: NSIG' >scn.c
	NSIG=`vq "$CPP $CPPFLAGS scn.c" | grep mksh_cfg: | \
	    sed 's/^mksh_cfg:[	 ]*\([0-9x ()+-]*\).*$/\1/'`
	case $NSIG in
	*[\ \(\)+-]*) NSIG=`awk "BEGIN { print $NSIG }"` ;;
	esac
	printf=printf
	(printf hallo) >/dev/null 2>&1 || printf=echo
	test $printf = echo || NSIG=`printf %d "$NSIG" 2>/dev/null`
	$printf "NSIG=$NSIG ... "
	sigs="ABRT ALRM BUS CHLD CLD CONT DIL EMT FPE HUP ILL INFO INT IO IOT"
	sigs="$sigs KILL LOST PIPE PROF PWR QUIT RESV SAK SEGV STOP SYS TERM"
	sigs="$sigs TRAP TSTP TTIN TTOU URG USR1 USR2 VTALRM WINCH XCPU XFSZ"
	test 1 = $HAVE_CPP_DD && test $NSIG -gt 1 && sigs="$sigs "`vq \
	    "$CPP $CPPFLAGS -dD scn.c" | grep '[	 ]SIG[A-Z0-9]*[	 ]' | \
	    sed 's/^\(.*[	 ]SIG\)\([A-Z0-9]*\)\([	 ].*\)$/\2/' | sort`
	test $NSIG -gt 1 || sigs=
	for name in $sigs; do
		echo '#include <signal.h>' >scn.c
		echo mksh_cfg: SIG$name >>scn.c
		vq "$CPP $CPPFLAGS scn.c" | grep mksh_cfg: | \
		    sed 's/^mksh_cfg:[	 ]*\([0-9x]*\).*$/\1:'$name/
	done | grep -v '^:' | while IFS=: read nr name; do
		test $printf = echo || nr=`printf %d "$nr" 2>/dev/null`
		test $nr -gt 0 && test $nr -le $NSIG || continue
		case $sigseen in
		*:$nr:*) ;;
		*)	echo "		{ $nr, \"$name\" },"
			sigseen=$sigseen$nr:
			$printf "$name=$nr " >&2
			;;
		esac
	done 2>&1 >signames.inc
	rm -f scn.c
	$e done.
fi

addsrcs HAVE_SETMODE setmode.c
addsrcs HAVE_STRLCPY strlcpy.c
test 1 = "$HAVE_CAN_VERB" && CFLAGS="$CFLAGS -verbose"
CPPFLAGS="$CPPFLAGS -DHAVE_CONFIG_H -DCONFIG_H_FILENAME=\\\"sh.h\\\""

objs=
case $curdir in
*\ *)	echo "#!./mksh" >test.sh ;;
*)	echo "#!$curdir/mksh" >test.sh ;;
esac
cat >>test.sh <<-EOF
	LC_ALL=C PATH='$PATH'; export LC_ALL PATH
	test -n "\$KSH_VERSION" || exit 1
	check_categories=$check_categories
	print Testing mksh for conformance:
	fgrep MirOS: '$srcdir/check.t'
	fgrep MIRBSD '$srcdir/check.t'
	print "This shell is actually:\\n\\t\$KSH_VERSION"
	print 'test.sh built for mksh $dstversion'
	cstr='\$os = defined \$^O ? \$^O : "unknown";'
	cstr="\$cstr"'print \$os . ", Perl version " . \$];'
	for perli in \$PERL perl5 perl no; do
		[[ \$perli = no ]] && exit 1
		perlos=\$(\$perli -e "\$cstr") 2>&- || continue
		print "Perl interpreter '\$perli' running on '\$perlos'"
		[[ -n \$perlos ]] && break
	done
	exec \$perli '$srcdir/check.pl' -s '$srcdir/check.t' -p '$curdir/mksh' -C \${check_categories#,} \$*$tsts
EOF
chmod 755 test.sh
if test x"$llvm" = x"NO"; then
	emitbc=-c
else
	emitbc="-emit-llvm -c"
fi
echo set -x >Rebuild.sh
for file in $SRCS; do
	objs="$objs `echo x"$file" | sed 's/^x\(.*\)\.c$/\1.o/'`"
	test -f $file || file=$srcdir/$file
	echo "$CC $CFLAGS $CPPFLAGS $emitbc $file || exit 1" >>Rebuild.sh
done
if test x"$llvm" = x"NO"; then
	lobjs=$objs
else
	echo "rm -f mksh.s" >>Rebuild.sh
	echo "llvm-link -o - $objs | opt $llvm | llc -o mksh.s" >>Rebuild.sh
	lobjs=mksh.s
fi
case $tcfn in
a.exe)	echo tcfn=mksh.exe >>Rebuild.sh ;;
*)	echo tcfn=mksh >>Rebuild.sh ;;
esac
echo "$CC $CFLAGS $LDFLAGS -o \$tcfn $lobjs $LIBS $ccpr" >>Rebuild.sh
echo 'test -f $tcfn || exit 1; size $tcfn' >>Rebuild.sh
if test 1 = $pm; then
	for file in $SRCS; do
		test -f $file || file=$srcdir/$file
		v "$CC $CFLAGS $CPPFLAGS $emitbc $file" &
	done
	wait
else
	for file in $SRCS; do
		test -f $file || file=$srcdir/$file
		v "$CC $CFLAGS $CPPFLAGS $emitbc $file" || exit 1
	done
fi
if test x"$emitbc" = x"-emit-llvm -c"; then
	rm -f mksh.s
	v "llvm-link -o - $objs | opt $llvm | llc -o mksh.s"
fi
case $tcfn in
a.exe)	tcfn=mksh.exe ;;
*)	tcfn=mksh ;;
esac
v "$CC $CFLAGS $LDFLAGS -o $tcfn $lobjs $LIBS $ccpr"
test -f $tcfn || exit 1
test 1 = $r || v "$NROFF -mdoc <'$srcdir/mksh.1' >mksh.cat1" || \
    rm -f mksh.cat1
test 0 = $eq && v size $tcfn
i=install
test -f /usr/ucb/$i && i=/usr/ucb/$i
test 1 = $eq && e=:
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
exit 0
