#!/bin/sh
# $MirOS: src/bin/mksh/genopt.sh,v 1.2 2013/11/17 22:22:51 tg Exp $
#-
# Copyright (c) 2013
#	Thorsten Glaser <tg@mirbsd.org>
#
# Provided that these terms and disclaimer and all copyright notices
# are retained or reproduced in an accompanying document, permission
# is granted to deal in this work without restriction, including un-
# limited rights to use, publicly perform, distribute, sell, modify,
# merge, give away, or sublicence.
#
# This work is provided "AS IS" and WITHOUT WARRANTY of any kind, to
# the utmost extent permitted by applicable law, neither express nor
# implied; without malicious intent or gross negligence. In no event
# may a licensor, author or contributor be held liable for indirect,
# direct, other damage, loss, or other issues arising in any way out
# of dealing in the work, even if advised of the possibility of such
# damage or existence of a defect, except proven that it results out
# of said person's immediate fault when using the work as intended.
#-
# Compile *.opt files to *.gen (F0, FN, getopt string) files.

LC_ALL=C
export LC_ALL

case $ZSH_VERSION:$VERSION in
:zsh*) ZSH_VERSION=2 ;;
esac

if test -n "${ZSH_VERSION+x}" && (emulate sh) >/dev/null 2>&1; then
	emulate sh
	NULLCMD=:
fi

if test -d /usr/xpg4/bin/. >/dev/null 2>&1; then
	# Solaris: some of the tools have weird behaviour, use portable ones
	PATH=/usr/xpg4/bin:$PATH
	export PATH
fi

allu=QWERTYUIOPASDFGHJKLZXCVBNM
alll=qwertyuiopasdfghjklzxcvbnm
nl='
'
me=`basename "$0"`
curdir=`pwd` srcdir=`dirname "$0" 2>/dev/null`
case x$srcdir in
x) srcdir=. ;;
*"'"*) echo Source directory must not contain single quotes.; exit 1 ;;
esac

die() {
	if test -n "$1"; then
		echo >&2 "E: $*"
		echo >&2 "E: in '$srcfile': '$line'"
	else
		echo >&2 "E: invalid input in '$srcfile': '$line'"
	fi
	rm -f "$bn.gen"
	exit 1
}

soptc() {
	optc=`echo "$line" | sed 's/^[<>]\(.\).*$/\1/'`
	test x"$optc" = x'|' && return
	optclo=`echo "$optc" | tr $allu $alll`
	if test x"$optc" = x"$optclo"; then
		islo=1
	else
		islo=0
	fi
	sym=`echo "$line" | sed 's/^[<>]/|/'`
	o_str=$o_str$nl"<$optclo$islo$sym"
}

scond() {
	case x$cond in
	x)
		cond=
		;;
	x*' '*)
		cond=`echo "$cond" | sed 's/^ //'`
		cond="#if $cond"
		;;
	x'!'*)
		cond=`echo "$cond" | sed 's/^!//'`
		cond="#ifndef $cond"
		;;
	x*)
		cond="#ifdef $cond"
		;;
	esac
}

for srcfile
do
	bn=`basename "$srcfile" | sed 's/.opt$//'`
	o_gen=
	o_str=
	o_sym=
	ddefs=
	state=0
	while IFS= read -r line; do
		case $state:$line in
		2:'|'*)
			# end of input
			o_sym=`echo "$line" | sed 's/^.//'`
			o_gen=$o_gen$nl"#undef F0"
			o_gen=$o_gen$nl"#undef FN"
			for sym in $ddefs; do
				o_gen=$o_gen$nl"#undef $sym"
			done
			state=3
			;;
		1:@@)
			# begin of data block
			o_gen=$o_gen$nl"#endif"
			o_gen=$o_gen$nl"#ifndef F0"
			o_gen=$o_gen$nl"#define F0 FN"
			o_gen=$o_gen$nl"#endif"
			state=2
			;;
		*:@@*)
			die ;;
		0:@*|1:@*)
			# begin of a definition block
			sym=`echo "$line" | sed 's/^@//'`
			if test $state = 0; then
				o_gen=$o_gen$nl"#if defined($sym)"
			else
				o_gen=$o_gen$nl"#elif defined($sym)"
			fi
			ddefs="$ddefs $sym"
			state=1
			;;
		0:*|3:*)
			die ;;
		1:*)
			# definition line
			o_gen=$o_gen$nl$line
			;;
		2:'<'*'|'*)
			soptc
			;;
		2:'>'*'|'*)
			soptc
			cond=`echo "$line" | sed 's/^[^|]*|//'`
			scond
			case $optc in
			'|') optc=0 ;;
			*) optc=\'$optc\' ;;
			esac
			IFS= read -r line || die Unexpected EOF
			test -n "$cond" && o_gen=$o_gen$nl"$cond"
			o_gen=$o_gen$nl"$line, $optc)"
			test -n "$cond" && o_gen=$o_gen$nl"#endif"
			;;
		esac
	done <"$srcfile"
	case $state:$o_sym in
	3:) die Expected optc sym at EOF ;;
	3:*) ;;
	*) die Missing EOF marker ;;
	esac
	echo "$o_str" | sort | while IFS='|' read -r x opts cond; do
		test -n "$x" || continue
		scond
		test -n "$cond" && echo "$cond"
		echo "\"$opts\""
		test -n "$cond" && echo "#endif"
	done | {
		echo "#ifndef $o_sym$o_gen"
		echo "#else"
		cat
		echo "#undef $o_sym"
		echo "#endif"
	} >"$bn.gen"
done
exit 0
