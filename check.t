# $MirOS: src/bin/mksh/check.t,v 1.158.2.5 2008/12/14 00:07:34 tg Exp $
# $OpenBSD: bksl-nl.t,v 1.2 2001/01/28 23:04:56 niklas Exp $
# $OpenBSD: history.t,v 1.5 2001/01/28 23:04:56 niklas Exp $
# $OpenBSD: read.t,v 1.3 2003/03/10 03:48:16 david Exp $
#-
# You may also want to test IFS with the script at
# http://www.research.att.com/~gsf/public/ifs.sh

expected-stdout:
	@(#)MIRBSD KSH R36 2008/12/13
description:
	Check version of shell.
stdin:
	echo $KSH_VERSION
name: KSH_VERSION
---
name: selftest-1
description:
	Regression test self-testing
stdin:
	print ${foo:-baz}
expected-stdout:
	baz
---
name: selftest-2
description:
	Regression test self-testing
env-setup: !foo=bar!
stdin:
	print ${foo:-baz}
expected-stdout:
	bar
---
name: selftest-3
description:
	Regression test self-testing
env-setup: !ENV=fnord!
stdin:
	print "<$ENV>"
expected-stdout:
	<fnord>
---
name: selftest-env
description:
	Just output the environment variables set (always fails)
category: disabled
stdin:
	set
---
name: alias-1
description:
	Check that recursion is detected/avoided in aliases.
stdin:
	alias fooBar=fooBar
	fooBar
	exit 0
expected-stderr-pattern:
	/fooBar.*not found.*/
---
name: alias-2
description:
	Check that recursion is detected/avoided in aliases.
stdin:
	alias fooBar=barFoo
	alias barFoo=fooBar
	fooBar
	barFoo
	exit 0
expected-stderr-pattern:
	/fooBar.*not found.*\n.*barFoo.*not found/
---
name: alias-3
description:
	Check that recursion is detected/avoided in aliases.
stdin:
	alias Echo='echo '
	alias fooBar=barFoo
	alias barFoo=fooBar
	Echo fooBar
	unalias barFoo
	Echo fooBar
expected-stdout:
	fooBar
	barFoo
---
name: alias-4
description:
	Check that alias expansion isn't done on keywords (in keyword
	postitions).
stdin:
	alias Echo='echo '
	alias while=While
	while false; do echo hi ; done
	Echo while
expected-stdout:
	While
---
name: alias-5
description:
	Check that alias expansion done after alias with trailing space.
stdin:
	alias Echo='echo '
	alias foo='bar stuff '
	alias bar='Bar1 Bar2 '
	alias stuff='Stuff'
	alias blah='Blah'
	Echo foo blah
expected-stdout:
	Bar1 Bar2 Stuff Blah
---
name: alias-6
description:
	Check that alias expansion done after alias with trailing space.
stdin:
	alias Echo='echo '
	alias foo='bar bar'
	alias bar='Bar '
	alias blah=Blah
	Echo foo blah
expected-stdout:
	Bar Bar Blah
---
name: alias-7
description:
	Check that alias expansion done after alias with trailing space
	after a keyword.
stdin:
	alias X='case '
	alias Y=Z
	X Y in 'Y') echo is y ;; Z) echo is z ; esac
expected-stdout:
	is z
---
name: alias-8
description:
	Check that newlines in an alias don't cause the command to be lost.
stdin:
	alias foo='
	
	
	echo hi
	
	
	
	echo there
	
	
	'
	foo
expected-stdout:
	hi
	there
---
name: alias-9
description:
	Check that recursion is detected/avoided in aliases.
	This check fails for slow machines or Cygwin, raise
	the time-limit clause (e.g. to 7) if this occurs.
time-limit: 3
stdin:
	echo -n >tf
	alias ls=ls
	ls
	echo $(ls)
	exit 0
expected-stdout:
	tf
	tf
---
name: arith-lazy-1
description:
	Check that only one side of ternary operator is evaluated
stdin:
	x=i+=2
	y=j+=2
	typeset -i i=1 j=1
	echo $((1 ? 20 : (x+=2)))
	echo $i,$x
	echo $((0 ? (y+=2) : 30))
	echo $j,$y
expected-stdout:
	20
	1,i+=2
	30
	1,j+=2
---
name: arith-lazy-2
description:
	Check that assignments not done on non-evaluated side of ternary
	operator
stdin:
	x=i+=2
	y=j+=2
	typeset -i i=1 j=1
	echo $((1 ? 20 : (x+=2)))
	echo $i,$x
	echo $((0 ? (y+=2) : 30))
	echo $i,$y
expected-stdout:
	20
	1,i+=2
	30
	1,j+=2
---
name: arith-ternary-prec-1
description:
	Check precidance of ternary operator vs assignment
stdin:
	typeset -i x=2
	y=$((1 ? 20 : x+=2))
expected-exit: e != 0
expected-stderr-pattern:
	/.*:.*1 \? 20 : x\+=2.*lvalue.*\n$/
---
name: arith-ternary-prec-2
description:
	Check precidance of ternary operator vs assignment
stdin:
	typeset -i x=2
	echo $((0 ? x+=2 : 20))
expected-stdout:
	20
---
name: arith-div-assoc-1
description:
	Check associativity of division operator
stdin:
	echo $((20 / 2 / 2))
expected-stdout:
	5
---
name: arith-assop-assoc-1
description:
	Check associativity of assignment-operator operator
stdin:
	typeset -i i=1 j=2 k=3
	echo $((i += j += k))
	echo $i,$j,$k
expected-stdout:
	6
	6,5,3
---
name: bksl-nl-ign-1
description:
	Check that \newline is not collasped after #
stdin:
	echo hi #there \
	echo folks
expected-stdout:
	hi
	folks
---
name: bksl-nl-ign-2
description:
	Check that \newline is not collasped inside single quotes
stdin:
	echo 'hi \
	there'
	echo folks
expected-stdout:
	hi \
	there
	folks
---
name: bksl-nl-ign-3
description:
	Check that \newline is not collasped inside single quotes
stdin:
	cat << \EOF
	hi \
	there
	EOF
expected-stdout:
	hi \
	there
---
name: bksl-nl-ign-4
description:
	Check interaction of aliases, single quotes and here-documents
	with backslash-newline
	(don't know what posix has to say about this)
stdin:
	a=2
	alias x='echo hi
	cat << "EOF"
	foo\
	bar
	some'
	x
	more\
	stuff$a
	EOF
expected-stdout:
	hi
	foo\
	bar
	some
	more\
	stuff$a
---
name: bksl-nl-ign-5
description:
	Check what happens with backslash at end of input
	(the old bourne shell trashes them; so do we)
stdin: !
	echo `echo foo\\`bar
	echo hi\
expected-stdout:
	foobar
	hi
---
#
# Places \newline should be collapsed
#
name: bksl-nl-1
description:
	Check that \newline is collasped before, in the middle of, and
	after words
stdin:
	 	 	\
			 echo hi\
	There, \
	folks
expected-stdout:
	hiThere, folks
---
name: bksl-nl-2
description:
	Check that \newline is collasped in $ sequences
	(ksh93 fails this)
stdin:
	a=12
	ab=19
	echo $\
	a
	echo $a\
	b
	echo $\
	{a}
	echo ${a\
	b}
	echo ${ab\
	}
expected-stdout:
	12
	19
	12
	19
	19
---
name: bksl-nl-3
description:
	Check that \newline is collasped in $(..) and `...` sequences
	(ksh93 fails this)
stdin:
	echo $\
	(echo foobar1)
	echo $(\
	echo foobar2)
	echo $(echo foo\
	bar3)
	echo $(echo foobar4\
	)
	echo `
	echo stuff1`
	echo `echo st\
	uff2`
expected-stdout:
	foobar1
	foobar2
	foobar3
	foobar4
	stuff1
	stuff2
---
name: bksl-nl-4
description:
	Check that \newline is collasped in $((..)) sequences
	(ksh93 fails this)
stdin:
	echo $\
	((1+2))
	echo $(\
	(1+2+3))
	echo $((\
	1+2+3+4))
	echo $((1+\
	2+3+4+5))
	echo $((1+2+3+4+5+6)\
	)
expected-stdout:
	3
	6
	10
	15
	21
---
name: bksl-nl-5
description:
	Check that \newline is collasped in double quoted strings
stdin:
	echo "\
	hi"
	echo "foo\
	bar"
	echo "folks\
	"
expected-stdout:
	hi
	foobar
	folks
---
name: bksl-nl-6
description:
	Check that \newline is collasped in here document delimiters
	(ksh93 fails second part of this)
stdin:
	a=12
	cat << EO\
	F
	a=$a
	foo\
	bar
	EOF
	cat << E_O_F
	foo
	E_O_\
	F
	echo done
expected-stdout:
	a=12
	foobar
	foo
	done
---
name: bksl-nl-7
description:
	Check that \newline is collasped in double-quoted here-document
	delimiter.
stdin:
	a=12
	cat << "EO\
	F"
	a=$a
	foo\
	bar
	EOF
	echo done
expected-stdout:
	a=$a
	foo\
	bar
	done
---
name: bksl-nl-8
description:
	Check that \newline is collasped in various 2+ character tokens
	delimiter.
	(ksh93 fails this)
stdin:
	echo hi &\
	& echo there
	echo foo |\
	| echo bar
	cat <\
	< EOF
	stuff
	EOF
	cat <\
	<\
	- EOF
		more stuff
	EOF
	cat <<\
	EOF
	abcdef
	EOF
	echo hi >\
	> /dev/null
	echo $?
	i=1
	case $i in
	(\
	x|\
	1\
	) echo hi;\
	;
	(*) echo oops
	esac
expected-stdout:
	hi
	there
	foo
	stuff
	more stuff
	abcdef
	0
	hi
---
name: bksl-nl-9
description:
	Check that \ at the end of an alias is collapsed when followed
	by a newline
	(don't know what posix has to say about this)
stdin:
	alias x='echo hi\'
	x
	echo there
expected-stdout:
	hiecho there
---
name: bksl-nl-10
description:
	Check that \newline in a keyword is collapsed
stdin:
	i\
	f true; then\
	 echo pass; el\
	se echo fail; fi
expected-stdout:
	pass
---
#
# Places \newline should be collapsed (ksh extensions)
#
name: bksl-nl-ksh-1
description:
	Check that \newline is collapsed in extended globbing
	(ksh93 fails this)
stdin:
	xxx=foo
	case $xxx in
	(f*\
	(\
	o\
	)\
	) echo ok ;;
	*) echo bad
	esac
expected-stdout:
	ok
---
name: bksl-nl-ksh-2
description:
	Check that \newline is collapsed in ((...)) expressions
	(ksh93 fails this)
stdin:
	i=1
	(\
	(\
	i=i+2\
	)\
	)
	echo $i
expected-stdout:
	3
---
name: break-1
description:
	See if break breaks out of loops
stdin:
	for i in a b c; do echo $i; break; echo bad-$i; done
	echo end-1
	for i in a b c; do echo $i; break 1; echo bad-$i; done
	echo end-2
	for i in a b c; do
	    for j in x y z; do
		echo $i:$j
		break
		echo bad-$i
	    done
	    echo end-$i
	done
	echo end-3
expected-stdout:
	a
	end-1
	a
	end-2
	a:x
	end-a
	b:x
	end-b
	c:x
	end-c
	end-3
---
name: break-2
description:
	See if break breaks out of nested loops
stdin:
	for i in a b c; do
	    for j in x y z; do
		echo $i:$j
		break 2
		echo bad-$i
	    done
	    echo end-$i
	done
	echo end
expected-stdout:
	a:x
	end
---
name: break-3
description:
	What if break used outside of any loops
	(ksh88,ksh93 don't print error messages here)
stdin:
	break
expected-stderr-pattern:
	/.*break.*/
---
name: break-4
description:
	What if break N used when only N-1 loops
	(ksh88,ksh93 don't print error messages here)
stdin:
	for i in a b c; do echo $i; break 2; echo bad-$i; done
	echo end
expected-stdout:
	a
	end
expected-stderr-pattern:
	/.*break.*/
---
name: break-5
description:
	Error if break argument isn't a number
stdin:
	for i in a b c; do echo $i; break abc; echo more-$i; done
	echo end
expected-stdout:
	a
expected-exit: e != 0
expected-stderr-pattern:
	/.*break.*/
---
name: continue-1
description:
	See if continue continues loops
stdin:
	for i in a b c; do echo $i; continue; echo bad-$i ; done
	echo end-1
	for i in a b c; do echo $i; continue 1; echo bad-$i; done
	echo end-2
	for i in a b c; do
	    for j in x y z; do
		echo $i:$j
		continue
		echo bad-$i-$j
	    done
	    echo end-$i
	done
	echo end-3
expected-stdout:
	a
	b
	c
	end-1
	a
	b
	c
	end-2
	a:x
	a:y
	a:z
	end-a
	b:x
	b:y
	b:z
	end-b
	c:x
	c:y
	c:z
	end-c
	end-3
---
name: continue-2
description:
	See if continue breaks out of nested loops
stdin:
	for i in a b c; do
	    for j in x y z; do
		echo $i:$j
		continue 2
		echo bad-$i-$j
	    done
	    echo end-$i
	done
	echo end
expected-stdout:
	a:x
	b:x
	c:x
	end
---
name: continue-3
description:
	What if continue used outside of any loops
	(ksh88,ksh93 don't print error messages here)
stdin:
	continue
expected-stderr-pattern:
	/.*continue.*/
---
name: continue-4
description:
	What if continue N used when only N-1 loops
	(ksh88,ksh93 don't print error messages here)
stdin:
	for i in a b c; do echo $i; continue 2; echo bad-$i; done
	echo end
expected-stdout:
	a
	b
	c
	end
expected-stderr-pattern:
	/.*continue.*/
---
name: continue-5
description:
	Error if continue argument isn't a number
stdin:
	for i in a b c; do echo $i; continue abc; echo more-$i; done
	echo end
expected-stdout:
	a
expected-exit: e != 0
expected-stderr-pattern:
	/.*continue.*/
---
name: cd-history
description:
	Test someone's CD history package (uses arrays)
stdin:
	# go to known place before doing anything
	cd /
	
	alias cd=_cd
	function _cd
	{
		typeset -i cdlen i
		typeset t
	
		if [ $# -eq 0 ]
		then
			set -- $HOME
		fi
	
		if [ "$CDHISTFILE" -a -r "$CDHISTFILE" ] # if directory history exists
		then
			typeset CDHIST
			i=-1
			while read -r t			# read directory history file
			do
				CDHIST[i=i+1]=$t
			done <$CDHISTFILE
		fi
	
		if [ "${CDHIST[0]}" != "$PWD" -a "$PWD" != "" ]
		then
			_cdins				# insert $PWD into cd history
		fi
	
		cdlen=${#CDHIST[*]}			# number of elements in history
	
		case "$@" in
		-)					# cd to new dir
			if [ "$OLDPWD" = "" ] && ((cdlen>1))
			then
				'print' ${CDHIST[1]}
				'cd' ${CDHIST[1]}
				_pwd
			else
				'cd' $@
				_pwd
			fi
			;;
		-l)					# print directory list
			typeset -R3 num
			((i=cdlen))
			while (((i=i-1)>=0))
			do
				num=$i
				'print' "$num ${CDHIST[i]}"
			done
			return
			;;
		-[0-9]|-[0-9][0-9])			# cd to dir in list
			if (((i=${1#-})<cdlen))
			then
				'print' ${CDHIST[i]}
				'cd' ${CDHIST[i]}
				_pwd
			else
				'cd' $@
				_pwd
			fi
			;;
		-*)					# cd to matched dir in list
			t=${1#-}
			i=1
			while ((i<cdlen))
			do
				case ${CDHIST[i]} in
				*$t*)
					'print' ${CDHIST[i]}
					'cd' ${CDHIST[i]}
					_pwd
					break
					;;
				esac
				((i=i+1))
			done
			if ((i>=cdlen))
			then
				'cd' $@
				_pwd
			fi
			;;
		*)					# cd to new dir
			'cd' $@
			_pwd
			;;
		esac
	
		_cdins					# insert $PWD into cd history
	
		if [ "$CDHISTFILE" ]
		then
			cdlen=${#CDHIST[*]}		# number of elements in history
	
			i=0
			while ((i<cdlen))
			do
				'print' -r ${CDHIST[i]}	# update directory history
				((i=i+1))
			done >$CDHISTFILE
		fi
	}
	
	function _cdins					# insert $PWD into cd history
	{						# meant to be called only by _cd
		typeset -i i
	
		((i=0))
		while ((i<${#CDHIST[*]}))		# see if dir is already in list
		do
			if [ "${CDHIST[$i]}" = "$PWD" ]
			then
				break
			fi
			((i=i+1))
		done
	
		if ((i>22))				# limit max size of list
		then
			i=22
		fi
	
		while (((i=i-1)>=0))			# bump old dirs in list
		do
			CDHIST[i+1]=${CDHIST[i]}
		done
	
		CDHIST[0]=$PWD				# insert new directory in list
	}
	
	
	function _pwd
	{
		if [ -n "$ECD" ]
		then
			pwd 1>&6
		fi
	}
	# Start of test
	cd /tmp
	cd /bin
	cd /etc
	cd -
	cd -2
	cd -l
expected-stdout:
	/bin
	/tmp
	  3 /
	  2 /etc
	  1 /bin
	  0 /tmp
---
name: env-prompt
description:
	Check that prompt not printed when processing ENV
env-setup: !ENV=./foo!
file-setup: file 644 "foo"
	XXX=_
	PS1=X
	false && echo hmmm
arguments: !-i!
stdin:
	echo hi${XXX}there
expected-stdout:
	hi_there
expected-stderr: !
	XX
---
name: eglob-bad-1
description:
	Check that globbing isn't done when glob has syntax error
file-setup: file 644 "abcx"
file-setup: file 644 "abcz"
file-setup: file 644 "bbc"
stdin:
	echo !([*)*
	echo +(a|b[)*
expected-stdout:
	!([*)*
	+(a|b[)*
---
name: eglob-bad-2
description:
	Check that globbing isn't done when glob has syntax error
	(at&t ksh fails this test)
file-setup: file 644 "abcx"
file-setup: file 644 "abcz"
file-setup: file 644 "bbc"
stdin:
	echo [a*(]*)z
expected-stdout:
	[a*(]*)z
---
name: eglob-infinite-plus
description:
	Check that shell doesn't go into infinite loop expanding +(...)
	expressions.
file-setup: file 644 "abc"
time-limit: 3
stdin:
	echo +()c
	echo +()x
	echo +(*)c
	echo +(*)x
expected-stdout:
	+()c
	+()x
	abc
	+(*)x
---
name: eglob-subst-1
description:
	Check that eglobbing isn't done on substitution results
file-setup: file 644 "abc"
stdin:
	x='@(*)'
	echo $x
expected-stdout:
	@(*)
---
name: eglob-nomatch-1
description:
	Check that the pattern doesn't match
stdin:
	echo 1: no-file+(a|b)stuff
	echo 2: no-file+(a*(c)|b)stuff
	echo 3: no-file+((((c)))|b)stuff
expected-stdout:
	1: no-file+(a|b)stuff
	2: no-file+(a*(c)|b)stuff
	3: no-file+((((c)))|b)stuff
---
name: eglob-match-1
description:
	Check that the pattern matches correctly
file-setup: file 644 "abd"
file-setup: file 644 "acd"
file-setup: file 644 "abac"
stdin:
	echo 1: a+(b|c)d
	echo 2: a!(@(b|B))d
	echo 3: *(a(b|c))		# (...|...) can be used within X(..)
	echo 4: a[b*(foo|bar)]d		# patterns not special inside [...]
expected-stdout:
	1: abd acd
	2: acd
	3: abac
	4: abd
---
name: eglob-case-1
description:
	Simple negation tests
stdin:
	case foo in !(foo|bar)) echo yes;; *) echo no;; esac
	case bar in !(foo|bar)) echo yes;; *) echo no;; esac
expected-stdout:
	no
	no
---
name: eglob-case-2
description:
	Simple kleene tests
stdin:
	case foo in *(a|b[)) echo yes;; *) echo no;; esac
	case foo in *(a|b[)|f*) echo yes;; *) echo no;; esac
	case '*(a|b[)' in *(a|b[)) echo yes;; *) echo no;; esac
expected-stdout:
	no
	yes
	yes
---
name: eglob-trim-1
description:
	Eglobbing in trim expressions...
	(at&t ksh fails this - docs say # matches shortest string, ## matches
	longest...)
stdin:
	x=abcdef
	echo 1: ${x#a|abc}
	echo 2: ${x##a|abc}
	echo 3: ${x%def|f}
	echo 4: ${x%%f|def}
expected-stdout:
	1: bcdef
	2: def
	3: abcde
	4: abc
---
name: eglob-trim-2
description:
	Check eglobbing works in trims...
stdin:
	x=abcdef
	echo 1: ${x#*(a|b)cd}
	echo 2: "${x#*(a|b)cd}"
	echo 3: ${x#"*(a|b)cd"}
	echo 4: ${x#a(b|c)}
expected-stdout:
	1: ef
	2: ef
	3: abcdef
	4: cdef
---
name: eglob-substrpl-1
description:
	Check eglobbing works in substs... and they work at all
stdin:
	[[ -n $BASH_VERSION ]] && shopt -s extglob
	x=1222321_ab/cde_b/c_1221
	y=xyz
	echo 1: ${x/2}
	echo 2: ${x//2}
	echo 3: ${x/+(2)}
	echo 4: ${x//+(2)}
	echo 5: ${x/2/4}
	echo 6: ${x//2/4}
	echo 7: ${x/+(2)/4}
	echo 8: ${x//+(2)/4}
	echo 9: ${x/b/c/e/f}
	echo 10: ${x/b\/c/e/f}
	echo 11: ${x/b\/c/e\/f}
	echo 12: ${x/b\/c/e\\/f}
	echo 13: ${x/b\\/c/e\\/f}
	echo 14: ${x//b/c/e/f}
	echo 15: ${x//b\/c/e/f}
	echo 16: ${x//b\/c/e\/f}
	echo 17: ${x//b\/c/e\\/f}
	echo 18: ${x//b\\/c/e\\/f}
	echo 19: ${x/b\/*\/c/x}
	echo 20: ${x/\//.}
	echo 21: ${x//\//.}
	echo 22: ${x///.}
	echo 23: ${x//#1/9}
	echo 24: ${x//%1/9}
	echo 25: ${x//\%1/9}
	echo 26: ${x//\\%1/9}
	echo 27: ${x//\a/9}
	echo 28: ${x//\\a/9}
	echo 29: ${x/2/$y}
expected-stdout:
	1: 122321_ab/cde_b/c_1221
	2: 131_ab/cde_b/c_11
	3: 1321_ab/cde_b/c_1221
	4: 131_ab/cde_b/c_11
	5: 1422321_ab/cde_b/c_1221
	6: 1444341_ab/cde_b/c_1441
	7: 14321_ab/cde_b/c_1221
	8: 14341_ab/cde_b/c_141
	9: 1222321_ac/e/f/cde_b/c_1221
	10: 1222321_ae/fde_b/c_1221
	11: 1222321_ae/fde_b/c_1221
	12: 1222321_ae\/fde_b/c_1221
	13: 1222321_ab/cde_b/c_1221
	14: 1222321_ac/e/f/cde_c/e/f/c_1221
	15: 1222321_ae/fde_e/f_1221
	16: 1222321_ae/fde_e/f_1221
	17: 1222321_ae\/fde_e\/f_1221
	18: 1222321_ab/cde_b/c_1221
	19: 1222321_ax_1221
	20: 1222321_ab.cde_b/c_1221
	21: 1222321_ab.cde_b.c_1221
	22: 1222321_ab/cde_b/c_1221
	23: 9222321_ab/cde_b/c_1221
	24: 1222321_ab/cde_b/c_1229
	25: 1222321_ab/cde_b/c_1229
	26: 1222321_ab/cde_b/c_1221
	27: 1222321_9b/cde_b/c_1221
	28: 1222321_9b/cde_b/c_1221
	29: 1xyz22321_ab/cde_b/c_1221
---
name: eglob-substrpl-2
description:
	Check anchored substring replacement works, corner cases
stdin:
	foo=123
	echo 1: ${foo/#/x}
	echo 2: ${foo/%/x}
	echo 3: ${foo/#/}
	echo 4: ${foo/#}
	echo 5: ${foo/%/}
	echo 6: ${foo/%}
expected-stdout:
	1: x123
	2: 123x
	3: 123
	4: 123
	5: 123
	6: 123
---
name: eglob-substrpl-3a
description:
	Check substring replacement works with variables and slashes, too
stdin:
	pfx=/home/user
	wd=/home/user/tmp
	echo ${wd/#$pfx/~}
	echo ${wd/#\$pfx/~}
	echo ${wd/#"$pfx"/~}
	echo ${wd/#'$pfx'/~}
	echo ${wd/#"\$pfx"/~}
	echo ${wd/#'\$pfx'/~}
expected-stdout:
	~/tmp
	/home/user/tmp
	~/tmp
	/home/user/tmp
	/home/user/tmp
	/home/user/tmp
---
name: eglob-substrpl-3b
description:
	More of this, bash fails it
stdin:
	pfx=/home/user
	wd=/home/user/tmp
	echo ${wd/#$(echo /home/user)/~}
	echo ${wd/#"$(echo /home/user)"/~}
	echo ${wd/#'$(echo /home/user)'/~}
expected-stdout:
	~/tmp
	~/tmp
	/home/user/tmp
---
name: eglob-substrpl-3c
description:
	Even more weird cases
stdin:
	pfx=/home/user
	wd='$pfx/tmp'
	echo 1: ${wd/#$pfx/~}
	echo 2: ${wd/#\$pfx/~}
	echo 3: ${wd/#"$pfx"/~}
	echo 4: ${wd/#'$pfx'/~}
	echo 5: ${wd/#"\$pfx"/~}
	echo 6: ${wd/#'\$pfx'/~}
	ts='a/ba/b$tp$tp_a/b$tp_*(a/b)_*($tp)'
	tp=a/b
	tr=c/d
	[[ -n $BASH_VERSION ]] && shopt -s extglob
	echo 7: ${ts/a\/b/$tr}
	echo 8: ${ts/a\/b/\$tr}
	echo 9: ${ts/$tp/$tr}
	echo 10: ${ts/\$tp/$tr}
	echo 11: ${ts/\\$tp/$tr}
	echo 12: ${ts/$tp/c/d}
	echo 13: ${ts/$tp/c\/d}
	echo 14: ${ts/$tp/c\\/d}
	echo 15: ${ts/+(a\/b)/$tr}
	echo 16: ${ts/+(a\/b)/\$tr}
	echo 17: ${ts/+($tp)/$tr}
	echo 18: ${ts/+($tp)/c/d}
	echo 19: ${ts/+($tp)/c\/d}
	echo 25: ${ts//a\/b/$tr}
	echo 26: ${ts//a\/b/\$tr}
	echo 27: ${ts//$tp/$tr}
	echo 28: ${ts//$tp/c/d}
	echo 29: ${ts//$tp/c\/d}
	echo 30: ${ts//+(a\/b)/$tr}
	echo 31: ${ts//+(a\/b)/\$tr}
	echo 32: ${ts//+($tp)/$tr}
	echo 33: ${ts//+($tp)/c/d}
	echo 34: ${ts//+($tp)/c\/d}
	tp="+($tp)"
	echo 40: ${ts/$tp/$tr}
	echo 41: ${ts//$tp/$tr}
expected-stdout:
	1: $pfx/tmp
	2: ~/tmp
	3: $pfx/tmp
	4: ~/tmp
	5: ~/tmp
	6: ~/tmp
	7: c/da/b$tp$tp_a/b$tp_*(a/b)_*($tp)
	8: $tra/b$tp$tp_a/b$tp_*(a/b)_*($tp)
	9: c/da/b$tp$tp_a/b$tp_*(a/b)_*($tp)
	10: a/ba/bc/d$tp_a/b$tp_*(a/b)_*($tp)
	11: c/da/b$tp$tp_a/b$tp_*(a/b)_*($tp)
	12: c/da/b$tp$tp_a/b$tp_*(a/b)_*($tp)
	13: c/da/b$tp$tp_a/b$tp_*(a/b)_*($tp)
	14: c\/da/b$tp$tp_a/b$tp_*(a/b)_*($tp)
	15: c/d$tp$tp_a/b$tp_*(a/b)_*($tp)
	16: $tr$tp$tp_a/b$tp_*(a/b)_*($tp)
	17: c/d$tp$tp_a/b$tp_*(a/b)_*($tp)
	18: c/d$tp$tp_a/b$tp_*(a/b)_*($tp)
	19: c/d$tp$tp_a/b$tp_*(a/b)_*($tp)
	25: c/dc/d$tp$tp_c/d$tp_*(c/d)_*($tp)
	26: $tr$tr$tp$tp_$tr$tp_*($tr)_*($tp)
	27: c/dc/d$tp$tp_c/d$tp_*(c/d)_*($tp)
	28: c/dc/d$tp$tp_c/d$tp_*(c/d)_*($tp)
	29: c/dc/d$tp$tp_c/d$tp_*(c/d)_*($tp)
	30: c/d$tp$tp_c/d$tp_*(c/d)_*($tp)
	31: $tr$tp$tp_$tr$tp_*($tr)_*($tp)
	32: c/d$tp$tp_c/d$tp_*(c/d)_*($tp)
	33: c/d$tp$tp_c/d$tp_*(c/d)_*($tp)
	34: c/d$tp$tp_c/d$tp_*(c/d)_*($tp)
	40: a/ba/b$tp$tp_a/b$tp_*(a/b)_*($tp)
	41: a/ba/b$tp$tp_a/b$tp_*(a/b)_*($tp)
#	This is what GNU bash does:
#	40: c/d$tp$tp_a/b$tp_*(a/b)_*($tp)
#	41: c/d$tp$tp_c/d$tp_*(c/d)_*($tp)
---
name: glob-bad-1
description:
	Check that globbing isn't done when glob has syntax error
file-setup: dir 755 "[x"
file-setup: file 644 "[x/foo"
stdin:
	echo [*
	echo *[x
	echo [x/*
expected-stdout:
	[*
	*[x
	[x/foo
---
name: glob-bad-2
description:
	Check that symbolic links aren't stat()'d
file-setup: dir 755 "dir"
file-setup: symlink 644 "dir/abc"
	non-existent-file
stdin:
	echo d*/*
	echo d*/abc
expected-stdout:
	dir/abc
	dir/abc
---
name: glob-range-1
description:
	Test range matching
file-setup: file 644 ".bc"
file-setup: file 644 "abc"
file-setup: file 644 "bbc"
file-setup: file 644 "cbc"
file-setup: file 644 "-bc"
stdin:
	echo [ab-]*
	echo [-ab]*
	echo [!-ab]*
	echo [!ab]*
	echo []ab]*
expected-stdout:
	-bc abc bbc
	-bc abc bbc
	cbc
	-bc cbc
	abc bbc
---
name: glob-range-2
description:
	Test range matching
	(at&t ksh fails this; POSIX says invalid)
file-setup: file 644 "abc"
stdin:
	echo [a--]*
expected-stdout:
	[a--]*
---
name: glob-range-3
description:
	Check that globbing matches the right things...
# breaks on Mac OSX (HFS+ non-standard Unicode canonical decomposition)
category: !os:darwin
file-setup: file 644 "a�c"
stdin:
	echo a[�-�]*
expected-stdout:
	a�c
---
name: glob-range-4
description:
	Results unspecified according to POSIX
file-setup: file 644 ".bc"
stdin:
	echo [a.]*
expected-stdout:
	[a.]*
---
name: glob-range-5
description:
	Results unspecified according to POSIX
	(at&t ksh treats this like [a-cc-e]*)
file-setup: file 644 "abc"
file-setup: file 644 "bbc"
file-setup: file 644 "cbc"
file-setup: file 644 "dbc"
file-setup: file 644 "ebc"
file-setup: file 644 "-bc"
stdin:
	echo [a-c-e]*
expected-stdout:
	-bc abc bbc cbc ebc
---
name: heredoc-1
description:
	Check ordering/content of redundent here documents.
stdin:
	cat << EOF1 << EOF2
	hi
	EOF1
	there
	EOF2
expected-stdout:
	there
---
name: heredoc-2
description:
	Check quoted here-doc is protected.
stdin:
	a=foo
	cat << 'EOF'
	hi\
	there$a
	stuff
	EO\
	F
	EOF
expected-stdout:
	hi\
	there$a
	stuff
	EO\
	F
---
name: heredoc-3
description:
	Check that newline isn't needed after heredoc-delimiter marker.
stdin: !
	cat << EOF
	hi
	there
	EOF
expected-stdout:
	hi
	there
---
name: heredoc-4
description:
	Check that an error occurs if the heredoc-delimiter is missing.
stdin: !
	cat << EOF
	hi
	there
expected-exit: e > 0
expected-stderr-pattern: /.*/
---
name: heredoc-5
description:
	Check that backslash quotes a $, ` and \ and kills a \newline
stdin:
	a=BAD
	b=ok
	cat << EOF
	h\${a}i
	h\\${b}i
	th\`echo not-run\`ere
	th\\`echo is-run`ere
	fol\\ks
	more\\
	last \
	line
	EOF
expected-stdout:
	h${a}i
	h\oki
	th`echo not-run`ere
	th\is-runere
	fol\ks
	more\
	last line
---
name: heredoc-6
description:
	Check that \newline in initial here-delim word doesn't imply
	a quoted here-doc.
stdin:
	a=i
	cat << EO\
	F
	h$a
	there
	EOF
expected-stdout:
	hi
	there
---
name: heredoc-7
description:
	Check that double quoted $ expressions in here delimiters are
	not expanded and match the delimiter.
	POSIX says only quote removal is applied to the delimiter.
stdin:
	a=b
	cat << "E$a"
	hi
	h$a
	hb
	E$a
	echo done
expected-stdout:
	hi
	h$a
	hb
	done
---
name: heredoc-8
description:
	Check that double quoted escaped $ expressions in here
	delimiters are not expanded and match the delimiter.
	POSIX says only quote removal is applied to the delimiter
	(\ counts as a quote).
stdin:
	a=b
	cat << "E\$a"
	hi
	h$a
	h\$a
	hb
	h\b
	E$a
	echo done
expected-stdout:
	hi
	h$a
	h\$a
	hb
	h\b
	done
---
name: heredoc-9a
description:
	Check that here strings work.
stdin:
	bar="bar
		baz"
	tr abcdefghijklmnopqrstuvwxyz nopqrstuvwxyzabcdefghijklm <<<foo
	"$__progname" -c "tr abcdefghijklmnopqrstuvwxyz nopqrstuvwxyzabcdefghijklm <<<foo"
	tr abcdefghijklmnopqrstuvwxyz nopqrstuvwxyzabcdefghijklm <<<"$bar"
	tr abcdefghijklmnopqrstuvwxyz nopqrstuvwxyzabcdefghijklm <<<'$bar'
	tr abcdefghijklmnopqrstuvwxyz nopqrstuvwxyzabcdefghijklm <<<\$bar
	tr abcdefghijklmnopqrstuvwxyz nopqrstuvwxyzabcdefghijklm <<<-foo
expected-stdout:
	sbb
	sbb
	one
		onm
	$one
	$one
	-sbb
---
name: heredoc-9b
description:
	Check that a corner case of here strings works like bash
stdin:
	fnord=42
	bar="bar
		 \$fnord baz"
	tr abcdefghijklmnopqrstuvwxyz nopqrstuvwxyzabcdefghijklm <<<$bar
expected-stdout:
	one $sabeq onm
category: bash
---
name: heredoc-9c
description:
	Check that a corner case of here strings works like ksh93, zsh
stdin:
	fnord=42
	bar="bar
		 \$fnord baz"
	tr abcdefghijklmnopqrstuvwxyz nopqrstuvwxyzabcdefghijklm <<<$bar
expected-stdout:
	one
		 $sabeq onm
---
name: heredoc-9d
description:
	Check another corner case of here strings
stdin:
	tr abcdefghijklmnopqrstuvwxyz nopqrstuvwxyzabcdefghijklm <<< bar
expected-stdout:
	one
---
name: heredoc-quoting-unsubst
description:
	Check for correct handling of quoted characters in
	here documents without substitution (marker is quoted).
stdin:
	foo=bar
	cat <<-'EOF'
		x " \" \ \\ $ \$ `echo baz` \`echo baz\` $foo \$foo x
	EOF
expected-stdout:
	x " \" \ \\ $ \$ `echo baz` \`echo baz\` $foo \$foo x
---
name: heredoc-quoting-subst
description:
	Check for correct handling of quoted characters in
	here documents with substitution (marker is not quoted).
stdin:
	foo=bar
	cat <<-EOF
		x " \" \ \\ $ \$ `echo baz` \`echo baz\` $foo \$foo x
	EOF
expected-stdout:
	x " \" \ \ $ $ baz `echo baz` bar $foo x
---
name: heredoc-tmpfile-1
description:
	Check that heredoc temp files aren't removed too soon or too late.
	Heredoc in simple command.
stdin:
	TMPDIR=$PWD
	eval '
		cat <<- EOF
		hi
		EOF
		for i in a b ; do
			cat <<- EOF
			more
			EOF
		done
	    ' &
	sleep 1
	echo Left overs: *
expected-stdout:
	hi
	more
	more
	Left overs: *
---
name: heredoc-tmpfile-2
description:
	Check that heredoc temp files aren't removed too soon or too late.
	Heredoc in function, multiple calls to function.
stdin:
	TMPDIR=$PWD
	eval '
		foo() {
			cat <<- EOF
			hi
			EOF
		}
		foo
		foo
	    ' &
	sleep 1
	echo Left overs: *
expected-stdout:
	hi
	hi
	Left overs: *
---
name: heredoc-tmpfile-3
description:
	Check that heredoc temp files aren't removed too soon or too late.
	Heredoc in function in loop, multiple calls to function.
stdin:
	TMPDIR=$PWD
	eval '
		foo() {
			cat <<- EOF
			hi
			EOF
		}
		for i in a b; do
			foo
			foo() {
				cat <<- EOF
				folks $i
				EOF
			}
		done
		foo
	    ' &
	sleep 1
	echo Left overs: *
expected-stdout:
	hi
	folks b
	folks b
	Left overs: *
---
name: heredoc-tmpfile-4
description:
	Check that heredoc temp files aren't removed too soon or too late.
	Backgrounded simple command with here doc
stdin:
	TMPDIR=$PWD
	eval '
		cat <<- EOF &
		hi
		EOF
	    ' &
	sleep 1
	echo Left overs: *
expected-stdout:
	hi
	Left overs: *
---
name: heredoc-tmpfile-5
description:
	Check that heredoc temp files aren't removed too soon or too late.
	Backgrounded subshell command with here doc
stdin:
	TMPDIR=$PWD
	eval '
	      (
		sleep 1	# so parent exits
		echo A
		cat <<- EOF
		hi
		EOF
		echo B
	      ) &
	    ' &
	sleep 2
	echo Left overs: *
expected-stdout:
	A
	hi
	B
	Left overs: *
---
name: heredoc-tmpfile-6
description:
	Check that heredoc temp files aren't removed too soon or too late.
	Heredoc in pipeline.
stdin:
	TMPDIR=$PWD
	eval '
		cat <<- EOF | sed "s/hi/HI/"
		hi
		EOF
	    ' &
	sleep 1
	echo Left overs: *
expected-stdout:
	HI
	Left overs: *
---
name: heredoc-tmpfile-7
description:
	Check that heredoc temp files aren't removed too soon or too late.
	Heredoc in backgrounded pipeline.
stdin:
	TMPDIR=$PWD
	eval '
		cat <<- EOF | sed 's/hi/HI/' &
		hi
		EOF
	    ' &
	sleep 1
	echo Left overs: *
expected-stdout:
	HI
	Left overs: *
---
name: heredoc-tmpfile-8
description:
	Check that heredoc temp files aren't removed too soon or too
	late. Heredoc in function, backgrounded call to function.
	This check can fail on slow machines (<100 MHz), or Cygwin,
	that's normal.
stdin:
	TMPDIR=$PWD
	# Background eval so main shell doesn't do parsing
	eval '
		foo() {
			cat <<- EOF
			hi
			EOF
		}
		foo
		# sleep so eval can die
		(sleep 1; foo) &
		(sleep 1; foo) &
		foo
	    ' &
	sleep 2
	echo Left overs: *
expected-stdout:
	hi
	hi
	hi
	hi
	Left overs: *
---
name: history-basic
description:
	See if we can test history at all
arguments: !-i!
env-setup: !ENV=./Env!HISTFILE=hist.file!
file-setup: file 644 "Env"
	PS1=X
stdin:
	echo hi
	fc -l
expected-stdout:
	hi
	1	echo hi
expected-stderr-pattern:
	/^X*$/
---
name: history-dups
description:
	Verify duplicates and spaces are not entered
arguments: !-i!
env-setup: !ENV=./Env!HISTFILE=hist.file!
file-setup: file 644 "Env"
	PS1=X
stdin:
	echo hi
	 echo yo
	echo hi
	fc -l
expected-stdout:
	hi
	yo
	hi
	1	echo hi
expected-stderr-pattern:
	/^X*$/
---
name: history-unlink
description:
	Check if broken HISTFILEs do not cause trouble
category: !os:cygwin
arguments: !-i!
env-setup: !ENV=./Env!HISTFILE=foo/hist.file!
file-setup: file 644 "Env"
	PS1=X
file-setup: dir 755 "foo"
file-setup: file 644 "foo/hist.file"
	sometext
time-limit: 5
perl-setup: chmod(0555, "foo");
stdin:
	echo hi
	fc -l
	chmod 0755 foo
expected-stdout:
	hi
	1	echo hi
expected-stderr-pattern:
	/(.*cannot unlink HISTFILE.*\n)?X*$/
---
name: history-e-minus-1
description:
	Check if more recent command is executed
arguments: !-i!
env-setup: !ENV=./Env!HISTFILE=hist.file!
file-setup: file 644 "Env"
	PS1=X
stdin:
	echo hi
	echo there
	fc -e -
expected-stdout:
	hi
	there
	there
expected-stderr-pattern:
	/^X*echo there\nX*$/
---
name: history-e-minus-2
description:
	Check that repeated command is printed before command
	is re-executed.
arguments: !-i!
env-setup: !ENV=./Env!HISTFILE=hist.file!
file-setup: file 644 "Env"
	PS1=X
stdin:
	exec 2>&1
	echo hi
	echo there
	fc -e -
expected-stdout-pattern:
	/X*hi\nX*there\nX*echo there\nthere\nX*/
expected-stderr-pattern:
	/^X*$/
---
name: history-e-minus-3
description:
	fc -e - fails when there is no history
	(ksh93 has a bug that causes this to fail)
	(ksh88 loops on this)
arguments: !-i!
env-setup: !ENV=./Env!HISTFILE=hist.file!
file-setup: file 644 "Env"
	PS1=X
stdin:
	fc -e -
	echo ok
expected-stdout:
	ok
expected-stderr-pattern:
	/^X*.*:.*history.*\nX*$/
---
name: history-e-minus-4
description:
	Check if "fc -e -" command output goes to stdout.
arguments: !-i!
env-setup: !ENV=./Env!HISTFILE=hist.file!
file-setup: file 644 "Env"
	PS1=X
stdin:
	echo abc
	fc -e - | (read x; echo "A $x")
	echo ok
expected-stdout:
	abc
	A abc
	ok
expected-stderr-pattern:
	/^X*echo abc\nX*/
---
name: history-e-minus-5
description:
	fc is replaced in history by new command.
arguments: !-i!
env-setup: !ENV=./Env!HISTFILE=hist.file!
file-setup: file 644 "Env"
	PS1=X
stdin:
	echo abc def
	echo ghi jkl
	:
	fc -e - echo
	fc -l 2 5
expected-stdout:
	abc def
	ghi jkl
	ghi jkl
	2	echo ghi jkl
	3	:
	4	echo ghi jkl
	5	fc -l 2 5
expected-stderr-pattern:
	/^X*echo ghi jkl\nX*$/
---
name: history-list-1
description:
	List lists correct range
	(ksh88 fails 'cause it lists the fc command)
arguments: !-i!
env-setup: !ENV=./Env!HISTFILE=hist.file!
file-setup: file 644 "Env"
	PS1=X
stdin:
	echo line 1
	echo line 2
	echo line 3
	fc -l -- -2
expected-stdout:
	line 1
	line 2
	line 3
	2	echo line 2
	3	echo line 3
expected-stderr-pattern:
	/^X*$/
---
name: history-list-2
description:
	Lists oldest history if given pre-historic number
	(ksh93 has a bug that causes this to fail)
	(ksh88 fails 'cause it lists the fc command)
arguments: !-i!
env-setup: !ENV=./Env!HISTFILE=hist.file!
file-setup: file 644 "Env"
	PS1=X
stdin:
	echo line 1
	echo line 2
	echo line 3
	fc -l -- -40
expected-stdout:
	line 1
	line 2
	line 3
	1	echo line 1
	2	echo line 2
	3	echo line 3
expected-stderr-pattern:
	/^X*$/
---
name: history-list-3
description:
	Can give number 'options' to fc
arguments: !-i!
env-setup: !ENV=./Env!HISTFILE=hist.file!
file-setup: file 644 "Env"
	PS1=X
stdin:
	echo line 1
	echo line 2
	echo line 3
	echo line 4
	fc -l -3 -2
expected-stdout:
	line 1
	line 2
	line 3
	line 4
	2	echo line 2
	3	echo line 3
expected-stderr-pattern:
	/^X*$/
---
name: history-list-4
description:
	-1 refers to previous command
arguments: !-i!
env-setup: !ENV=./Env!HISTFILE=hist.file!
file-setup: file 644 "Env"
	PS1=X
stdin:
	echo line 1
	echo line 2
	echo line 3
	echo line 4
	fc -l -1 -1
expected-stdout:
	line 1
	line 2
	line 3
	line 4
	4	echo line 4
expected-stderr-pattern:
	/^X*$/
---
name: history-list-5
description:
	List command stays in history
arguments: !-i!
env-setup: !ENV=./Env!HISTFILE=hist.file!
file-setup: file 644 "Env"
	PS1=X
stdin:
	echo line 1
	echo line 2
	echo line 3
	echo line 4
	fc -l -1 -1
	fc -l -2 -1
expected-stdout:
	line 1
	line 2
	line 3
	line 4
	4	echo line 4
	4	echo line 4
	5	fc -l -1 -1
expected-stderr-pattern:
	/^X*$/
---
name: history-list-6
description:
	HISTSIZE limits about of history kept.
	(ksh88 fails 'cause it lists the fc command)
arguments: !-i!
env-setup: !ENV=./Env!HISTFILE=hist.file!HISTSIZE=3!
file-setup: file 644 "Env"
	PS1=X
stdin:
	echo line 1
	echo line 2
	echo line 3
	echo line 4
	echo line 5
	fc -l
expected-stdout:
	line 1
	line 2
	line 3
	line 4
	line 5
	4	echo line 4
	5	echo line 5
expected-stderr-pattern:
	/^X*$/
---
name: history-list-7
description:
	fc allows too old/new errors in range specification
arguments: !-i!
env-setup: !ENV=./Env!HISTFILE=hist.file!HISTSIZE=3!
file-setup: file 644 "Env"
	PS1=X
stdin:
	echo line 1
	echo line 2
	echo line 3
	echo line 4
	echo line 5
	fc -l 1 30
expected-stdout:
	line 1
	line 2
	line 3
	line 4
	line 5
	4	echo line 4
	5	echo line 5
	6	fc -l 1 30
expected-stderr-pattern:
	/^X*$/
---
name: history-list-r-1
description:
	test -r flag in history
arguments: !-i!
env-setup: !ENV=./Env!HISTFILE=hist.file!
file-setup: file 644 "Env"
	PS1=X
stdin:
	echo line 1
	echo line 2
	echo line 3
	echo line 4
	echo line 5
	fc -l -r 2 4
expected-stdout:
	line 1
	line 2
	line 3
	line 4
	line 5
	4	echo line 4
	3	echo line 3
	2	echo line 2
expected-stderr-pattern:
	/^X*$/
---
name: history-list-r-2
description:
	If first is newer than last, -r is implied.
arguments: !-i!
env-setup: !ENV=./Env!HISTFILE=hist.file!
file-setup: file 644 "Env"
	PS1=X
stdin:
	echo line 1
	echo line 2
	echo line 3
	echo line 4
	echo line 5
	fc -l 4 2
expected-stdout:
	line 1
	line 2
	line 3
	line 4
	line 5
	4	echo line 4
	3	echo line 3
	2	echo line 2
expected-stderr-pattern:
	/^X*$/
---
name: history-list-r-3
description:
	If first is newer than last, -r is cancelled.
arguments: !-i!
env-setup: !ENV=./Env!HISTFILE=hist.file!
file-setup: file 644 "Env"
	PS1=X
stdin:
	echo line 1
	echo line 2
	echo line 3
	echo line 4
	echo line 5
	fc -l -r 4 2
expected-stdout:
	line 1
	line 2
	line 3
	line 4
	line 5
	2	echo line 2
	3	echo line 3
	4	echo line 4
expected-stderr-pattern:
	/^X*$/
---
name: history-subst-1
description:
	Basic substitution
arguments: !-i!
env-setup: !ENV=./Env!HISTFILE=hist.file!
file-setup: file 644 "Env"
	PS1=X
stdin:
	echo abc def
	echo ghi jkl
	fc -e - abc=AB 'echo a'
expected-stdout:
	abc def
	ghi jkl
	AB def
expected-stderr-pattern:
	/^X*echo AB def\nX*$/
---
name: history-subst-2
description:
	Does subst find previous command?
arguments: !-i!
env-setup: !ENV=./Env!HISTFILE=hist.file!
file-setup: file 644 "Env"
	PS1=X
stdin:
	echo abc def
	echo ghi jkl
	fc -e - jkl=XYZQRT 'echo g'
expected-stdout:
	abc def
	ghi jkl
	ghi XYZQRT
expected-stderr-pattern:
	/^X*echo ghi XYZQRT\nX*$/
---
name: history-subst-3
description:
	Does subst find previous command when no arguments given
arguments: !-i!
env-setup: !ENV=./Env!HISTFILE=hist.file!
file-setup: file 644 "Env"
	PS1=X
stdin:
	echo abc def
	echo ghi jkl
	fc -e - jkl=XYZQRT
expected-stdout:
	abc def
	ghi jkl
	ghi XYZQRT
expected-stderr-pattern:
	/^X*echo ghi XYZQRT\nX*$/
---
name: history-subst-4
description:
	Global substitutions work
	(ksh88 and ksh93 do not have -g option)
arguments: !-i!
env-setup: !ENV=./Env!HISTFILE=hist.file!
file-setup: file 644 "Env"
	PS1=X
stdin:
	echo abc def asjj sadjhasdjh asdjhasd
	fc -e - -g a=FooBAR
expected-stdout:
	abc def asjj sadjhasdjh asdjhasd
	FooBARbc def FooBARsjj sFooBARdjhFooBARsdjh FooBARsdjhFooBARsd
expected-stderr-pattern:
	/^X*echo FooBARbc def FooBARsjj sFooBARdjhFooBARsdjh FooBARsdjhFooBARsd\nX*$/
---
name: history-subst-5
description:
	Make sure searches don't find current (fc) command
	(ksh88/ksh93 don't have the ? prefix thing so they fail this test)
arguments: !-i!
env-setup: !ENV=./Env!HISTFILE=hist.file!
file-setup: file 644 "Env"
	PS1=X
stdin:
	echo abc def
	echo ghi jkl
	fc -e - abc=AB \?abc
expected-stdout:
	abc def
	ghi jkl
	AB def
expected-stderr-pattern:
	/^X*echo AB def\nX*$/
---
name: history-ed-1-old
description:
	Basic (ed) editing works (assumes you have generic ed editor
	that prints no prompts). This is for oldish ed(1) which write
	the character count to stdout.
category: stdout-ed
arguments: !-i!
env-setup: !ENV=./Env!HISTFILE=hist.file!
file-setup: file 644 "Env"
	PS1=X
stdin:
	echo abc def
	fc echo
	s/abc/FOOBAR/
	w
	q
expected-stdout:
	abc def
	13
	16
	FOOBAR def
expected-stderr-pattern:
	/^X*echo FOOBAR def\nX*$/
---
name: history-ed-2-old
description:
	Correct command is edited when number given
category: stdout-ed
arguments: !-i!
env-setup: !ENV=./Env!HISTFILE=hist.file!
file-setup: file 644 "Env"
	PS1=X
stdin:
	echo line 1
	echo line 2 is here
	echo line 3
	echo line 4
	fc 2
	s/is here/is changed/
	w
	q
expected-stdout:
	line 1
	line 2 is here
	line 3
	line 4
	20
	23
	line 2 is changed
expected-stderr-pattern:
	/^X*echo line 2 is changed\nX*$/
---
name: history-ed-3-old
description:
	Newly created multi line commands show up as single command
	in history.
	(NOTE: adjusted for COMPLEX HISTORY compile time option)
	(ksh88 fails 'cause it lists the fc command)
category: stdout-ed
arguments: !-i!
env-setup: !ENV=./Env!HISTFILE=hist.file!
file-setup: file 644 "Env"
	PS1=X
stdin:
	echo abc def
	fc echo
	s/abc/FOOBAR/
	$a
	echo a new line
	.
	w
	q
	fc -l
expected-stdout:
	abc def
	13
	32
	FOOBAR def
	a new line
	1	echo abc def
	2	echo FOOBAR def
	3	echo a new line
expected-stderr-pattern:
	/^X*echo FOOBAR def\necho a new line\nX*$/
---
name: history-ed-1
description:
	Basic (ed) editing works (assumes you have generic ed editor
	that prints no prompts). This is for newish ed(1) and stderr.
category: !no-stderr-ed
arguments: !-i!
env-setup: !ENV=./Env!HISTFILE=hist.file!
file-setup: file 644 "Env"
	PS1=X
stdin:
	echo abc def
	fc echo
	s/abc/FOOBAR/
	w
	q
expected-stdout:
	abc def
	FOOBAR def
expected-stderr-pattern:
	/^X*13\n16\necho FOOBAR def\nX*$/
---
name: history-ed-2
description:
	Correct command is edited when number given
category: !no-stderr-ed
arguments: !-i!
env-setup: !ENV=./Env!HISTFILE=hist.file!
file-setup: file 644 "Env"
	PS1=X
stdin:
	echo line 1
	echo line 2 is here
	echo line 3
	echo line 4
	fc 2
	s/is here/is changed/
	w
	q
expected-stdout:
	line 1
	line 2 is here
	line 3
	line 4
	line 2 is changed
expected-stderr-pattern:
	/^X*20\n23\necho line 2 is changed\nX*$/
---
name: history-ed-3
description:
	Newly created multi line commands show up as single command
	in history.
category: !no-stderr-ed
arguments: !-i!
env-setup: !ENV=./Env!HISTFILE=hist.file!
file-setup: file 644 "Env"
	PS1=X
stdin:
	echo abc def
	fc echo
	s/abc/FOOBAR/
	$a
	echo a new line
	.
	w
	q
	fc -l
expected-stdout:
	abc def
	FOOBAR def
	a new line
	1	echo abc def
	2	echo FOOBAR def
	3	echo a new line
expected-stderr-pattern:
	/^X*13\n32\necho FOOBAR def\necho a new line\nX*$/
---
name: IFS-space-1
description:
	Simple test, default IFS
stdin:
	showargs() { for i; do echo -n " <$i>"; done; echo; }
	set -- A B C
	showargs 1 $*
	showargs 2 "$*"
	showargs 3 $@
	showargs 4 "$@"
expected-stdout:
	 <1> <A> <B> <C>
	 <2> <A B C>
	 <3> <A> <B> <C>
	 <4> <A> <B> <C>
---
name: IFS-colon-1
description:
	Simple test, IFS=:
stdin:
	showargs() { for i; do echo -n " <$i>"; done; echo; }
	IFS=:
	set -- A B C
	showargs 1 $*
	showargs 2 "$*"
	showargs 3 $@
	showargs 4 "$@"
expected-stdout:
	 <1> <A> <B> <C>
	 <2> <A:B:C>
	 <3> <A> <B> <C>
	 <4> <A> <B> <C>
---
name: IFS-null-1
description:
	Simple test, IFS=""
stdin:
	showargs() { for i; do echo -n " <$i>"; done; echo; }
	IFS=""
	set -- A B C
	showargs 1 $*
	showargs 2 "$*"
	showargs 3 $@
	showargs 4 "$@"
expected-stdout:
	 <1> <A B C>
	 <2> <ABC>
	 <3> <A B C>
	 <4> <A B C>
---
name: IFS-space-colon-1
description:
	Simple test, IFS=<white-space>:
stdin:
	showargs() { for i; do echo -n " <$i>"; done; echo; }
	IFS="$IFS:"
	set --
	showargs 1 $*
	showargs 2 "$*"
	showargs 3 $@
	showargs 4 "$@"
	showargs 5 : "$@"
expected-stdout:
	 <1>
	 <2> <>
	 <3>
	 <4>
	 <5> <:>
---
name: IFS-space-colon-2
description:
	Simple test, IFS=<white-space>:
	At&t ksh fails this, POSIX says the test is correct.
stdin:
	showargs() { for i; do echo -n " <$i>"; done; echo; }
	IFS="$IFS:"
	set --
	showargs :"$@"
expected-stdout:
	 <:>
---
name: IFS-space-colon-3
description:
	Simple test, IFS=<white-space>:
	pdksh fails both of these tests
	not sure whether #2 is correct
stdin:
	showargs() { for i; do echo -n " <$i>"; done; echo; }
	IFS="$IFS:"
	x=
	set --
	showargs "$x$@" 1
	showargs "$@$x" 2
expected-fail: yes
expected-stdout:
	 <> <1>
	 <> <2>
---
name: IFS-space-colon-4
description:
	Simple test, IFS=<white-space>:
stdin:
	showargs() { for i; do echo -n " <$i>"; done; echo; }
	IFS="$IFS:"
	set --
	showargs "$@$@"
expected-stdout:
	
---
name: IFS-space-colon-5
description:
	Simple test, IFS=<white-space>:
	Don't know what POSIX thinks of this.  at&t ksh does not do this.
stdin:
	showargs() { for i; do echo -n " <$i>"; done; echo; }
	IFS="$IFS:"
	set --
	showargs "${@:-}"
expected-stdout:
	 <>
---
name: IFS-subst-1
description:
	Simple test, IFS=<white-space>:
stdin:
	showargs() { for i; do echo -n " <$i>"; done; echo; }
	IFS="$IFS:"
	x=":b: :"
	echo -n '1:'; for i in $x ; do echo -n " [$i]" ; done ; echo
	echo -n '2:'; for i in :b:: ; do echo -n " [$i]" ; done ; echo
	showargs 3 $x
	showargs 4 :b::
	x="a:b:"
	echo -n '5:'; for i in $x ; do echo -n " [$i]" ; done ; echo
	showargs 6 $x
	x="a::c"
	echo -n '7:'; for i in $x ; do echo -n " [$i]" ; done ; echo
	showargs 8 $x
	echo -n '9:'; for i in ${FOO-`echo -n h:i`th:ere} ; do echo -n " [$i]" ; done ; echo
	showargs 10 ${FOO-`echo -n h:i`th:ere}
	showargs 11 "${FOO-`echo -n h:i`th:ere}"
	x=" A :  B::D"
	echo -n '12:'; for i in $x ; do echo -n " [$i]" ; done ; echo
	showargs 13 $x
expected-stdout:
	1: [] [b] []
	2: [:b::]
	 <3> <> <b> <>
	 <4> <:b::>
	5: [a] [b]
	 <6> <a> <b>
	7: [a] [] [c]
	 <8> <a> <> <c>
	9: [h] [ith] [ere]
	 <10> <h> <ith> <ere>
	 <11> <h:ith:ere>
	12: [A] [B] [] [D]
	 <13> <A> <B> <> <D>
---
name: integer-base-err-1
description:
	Can't have 0 base (causes shell to exit)
expected-exit: e != 0
stdin:
	typeset -i i
	i=3
	i=0#4
	echo $i
expected-stderr-pattern:
	/^.*:.*0#4.*\n$/
---
name: integer-base-err-2
description:
	Can't have multiple bases in a 'constant' (causes shell to exit)
	(ksh88 fails this test)
expected-exit: e != 0
stdin:
	typeset -i i
	i=3
	i=2#110#11
	echo $i
expected-stderr-pattern:
	/^.*:.*2#110#11.*\n$/
---
name: integer-base-err-3
description:
	Syntax errors in expressions and effects on bases
	(interactive so errors don't cause exits)
	(ksh88 fails this test - shell exits, even with -i)
arguments: !-i!
stdin:
	PS1= # minimise prompt hassles
	typeset -i4 a=10
	typeset -i a=2+
	echo $a
	typeset -i4 a=10
	typeset -i2 a=2+
	echo $a
expected-stderr-pattern:
	/^([#\$] )?.*:.*2+.*\n.*:.*2+.*\n$/
expected-stdout:
	4#22
	4#22
---
name: integer-base-err-4
description:
	Are invalid digits (according to base) errors?
	(ksh93 fails this test)
expected-exit: e != 0
stdin:
	typeset -i i;
	i=3#4
expected-stderr-pattern:
	/^([#\$] )?.*:.*3#4.*\n$/
---
name: integer-base-1
description:
	Missing number after base is treated as 0.
stdin:
	typeset -i i
	i=3
	i=2#
	echo $i
expected-stdout:
	0
---
name: integer-base-2
description:
	Check 'stickyness' of base in various situations
stdin:
	typeset -i i=8
	echo $i
	echo ---------- A
	typeset -i4 j=8
	echo $j
	echo ---------- B
	typeset -i k=8
	typeset -i4 k=8
	echo $k
	echo ---------- C
	typeset -i4 l
	l=3#10
	echo $l
	echo ---------- D
	typeset -i m
	m=3#10
	echo $m
	echo ---------- E
	n=2#11
	typeset -i n
	echo $n
	n=10
	echo $n
	echo ---------- F
	typeset -i8 o=12
	typeset -i4 o
	echo $o
	echo ---------- G
	typeset -i p
	let p=8#12
	echo $p
expected-stdout:
	8
	---------- A
	4#20
	---------- B
	4#20
	---------- C
	4#3
	---------- D
	3#10
	---------- E
	2#11
	2#1010
	---------- F
	4#30
	---------- G
	8#12
---
name: integer-base-3
description:
	More base parsing (hmm doesn't test much..)
stdin:
	typeset -i aa
	aa=1+12#10+2
	echo $aa
	typeset -i bb
	bb=1+$aa
	echo $bb
	typeset -i bb
	bb=$aa
	echo $bb
	typeset -i cc
	cc=$aa
	echo $cc
expected-stdout:
	15
	16
	15
	15
---
name: integer-base-4
description:
	Check that things not declared as integers are not made integers,
	also, check if base is not reset by -i with no arguments.
	(ksh93 fails - prints 10#20 - go figure)
stdin:
	xx=20
	let xx=10
	typeset -i | grep '^xx='
	typeset -i4 a=10
	typeset -i a=20
	echo $a
expected-stdout:
	4#110
---
name: integer-base-5
description:
	More base stuff
stdin:
	typeset -i4 a=3#10
	echo $a
	echo --
	typeset -i j=3
	j='~3'
	echo $j
	echo --
	typeset -i k=1
	x[k=k+1]=3
	echo $k
	echo --
	typeset -i l
	for l in 1 2+3 4; do echo $l; done
expected-stdout:
	4#3
	--
	-4
	--
	2
	--
	1
	5
	4
---
name: integer-base-6
description:
	Even more base stuff
	(ksh93 fails this test - prints 0)
stdin:
	typeset -i7 i
	i=
	echo $i
expected-stdout:
	7#0
---
name: integer-base-7
description:
	Check that non-integer parameters don't get bases assigned
stdin:
	echo $(( zz = 8#100 ))
	echo $zz
expected-stdout:
	64
	64
---
name: lineno-stdin
description:
	See if $LINENO is updated and can be modified.
stdin:
	echo A $LINENO
	echo B $LINENO
	LINENO=20
	echo C $LINENO
expected-stdout:
	A 1
	B 2
	C 20
---
name: lineno-inc
description:
	See if $LINENO is set for .'d files.
file-setup: file 644 "dotfile"
	echo dot A $LINENO
	echo dot B $LINENO
	LINENO=20
	echo dot C $LINENO
stdin:
	echo A $LINENO
	echo B $LINENO
	. ./dotfile
expected-stdout:
	A 1
	B 2
	dot A 1
	dot B 2
	dot C 20
---
name: lineno-func
description:
	See if $LINENO is set for commands in a function.
stdin:
	echo A $LINENO
	echo B $LINENO
	bar() {
	    echo func A $LINENO
	    echo func B $LINENO
	}
	bar
	echo C $LINENO
expected-stdout:
	A 1
	B 2
	func A 4
	func B 5
	C 8
---
name: lineno-unset
description:
	See if unsetting LINENO makes it non-magic.
file-setup: file 644 "dotfile"
	echo dot A $LINENO
	echo dot B $LINENO
stdin:
	unset LINENO
	echo A $LINENO
	echo B $LINENO
	bar() {
	    echo func A $LINENO
	    echo func B $LINENO
	}
	bar
	. ./dotfile
	echo C $LINENO
expected-stdout:
	A
	B
	func A
	func B
	dot A
	dot B
	C
---
name: lineno-unset-use
description:
	See if unsetting LINENO makes it non-magic even
	when it is re-used.
file-setup: file 644 "dotfile"
	echo dot A $LINENO
	echo dot B $LINENO
stdin:
	unset LINENO
	LINENO=3
	echo A $LINENO
	echo B $LINENO
	bar() {
	    echo func A $LINENO
	    echo func B $LINENO
	}
	bar
	. ./dotfile
	echo C $LINENO
expected-stdout:
	A 3
	B 3
	func A 3
	func B 3
	dot A 3
	dot B 3
	C 3
---
name: read-IFS-1
description:
	Simple test, default IFS
stdin:
	echo "A B " > IN
	unset x y z
	read x y z < IN
	echo 1: "x[$x] y[$y] z[$z]"
	echo 1a: ${z-z not set}
	read x < IN
	echo 2: "x[$x]"
expected-stdout:
	1: x[A] y[B] z[]
	1a:
	2: x[A B]
---
name: read-ksh-1
description:
	If no var specified, REPLY is used
stdin:
	echo "abc" > IN
	read < IN
	echo "[$REPLY]";
expected-stdout:
	[abc]
---
name: regression-1
description:
	Lex array code had problems with this.
stdin:
	echo foo[
	n=bar
	echo "hi[ $n ]=1"
expected-stdout:
	foo[
	hi[ bar ]=1
---
name: regression-2
description:
	When PATH is set before running a command, the new path is
	not used in doing the path search
		$ echo echo hi > /tmp/q ; chmod a+rx /tmp/q
		$ PATH=/tmp q
		q: not found
		$
	in comexec() the two lines
		while (*vp != NULL)
			(void) typeset(*vp++, xxx, 0);
	need to be moved out of the switch to before findcom() is
	called - I don't know what this will break.
stdin:
	: ${PWD:-`pwd 2> /dev/null`}
	: ${PWD:?"PWD not set - can't do test"}
	mkdir Y
	cat > Y/xxxscript << EOF
	#!/bin/sh
	# Need to restore path so echo can be found (some shells don't have
	# it as a built-in)
	PATH=\$OLDPATH
	echo hi
	exit 0
	EOF
	chmod a+rx Y/xxxscript
	export OLDPATH="$PATH"
	PATH=$PWD/Y xxxscript
	exit $?
expected-stdout:
	hi
---
name: regression-6
description:
	Parsing of $(..) expressions is non-optimal.  It is
	impossible to have any parentheses inside the expression.
	I.e.,
		$ ksh -c 'echo $(echo \( )'
		no closing quote
		$ ksh -c 'echo $(echo "(" )'
		no closing quote
		$
	The solution is to hack the parsing clode in lex.c, the
	question is how to hack it: should any parentheses be
	escaped by a backslash, or should recursive parsing be done
	(so quotes could also be used to hide hem).  The former is
	easier, the later better...
stdin:
	echo $(echo \()
expected-stdout:
	(
---
name: regression-9
description:
	Continue in a for loop does not work right:
		for i in a b c ; do
			if [ $i = b ] ; then
				continue
			fi
			echo $i
		done
	Prints a forever...
stdin:
	first=yes
	for i in a b c ; do
		if [ $i = b ] ; then
			if [ $first = no ] ; then
				echo 'continue in for loop broken'
				break	# hope break isn't broken too :-)
			fi
			first=no
			continue
		fi
	done
	echo bye
expected-stdout:
	bye
---
name: regression-10
description:
	The following:
		set -- `false`
		echo $?
	shoud not print 0. (according to /bin/sh, at&t ksh88, and the
	getopt(1) man page - not according to POSIX)
stdin:
	set -- `false`
	echo $?
expected-stdout:
	1
---
name: regression-11
description:
	The following:
		x=/foo/bar/blah
		echo ${x##*/}
	should echo blah but on some machines echos /foo/bar/blah.
stdin:
	x=/foo/bar/blah
	echo ${x##*/}
expected-stdout:
	blah
---
name: regression-12
description:
	Both of the following echos produce the same output under sh/ksh.att:
		#!/bin/sh
		x="foo	bar"
		echo "`echo \"$x\"`"
		echo "`echo "$x"`"
	pdksh produces different output for the former (foo instead of foo\tbar)
stdin:
	x="foo	bar"
	echo "`echo \"$x\"`"
	echo "`echo "$x"`"
expected-stdout:
	foo	bar
	foo	bar
---
name: regression-13
description:
	The following command hangs forever:
		$ (: ; cat /etc/termcap) | sleep 2
	This is because the shell forks a shell to run the (..) command
	and this shell has the pipe open.  When the sleep dies, the cat
	doesn't get a SIGPIPE 'cause a process (ie, the second shell)
	still has the pipe open.
	
	NOTE: this test provokes a bizarre bug in ksh93 (shell starts reading
	      commands from /etc/termcap..)
time-limit: 10
stdin:
	echo A line of text that will be duplicated quite a number of times.> t1
	cat t1 t1 t1 t1  t1 t1 t1 t1  t1 t1 t1 t1  t1 t1 t1 t1  > t2
	cat t2 t2 t2 t2  t2 t2 t2 t2  t2 t2 t2 t2  t2 t2 t2 t2  > t1
	cat t1 t1 t1 t1 > t2
	(: ; cat t2 2>&-) | sleep 1
---
name: regression-14
description:
	The command
		$ (foobar) 2> /dev/null
	generates no output under /bin/sh, but pdksh produces the error
		foobar: not found
	Also, the command
		$ foobar 2> /dev/null
	generates an error under /bin/sh and pdksh, but at&t ksh88 produces
	no error (redirected to /dev/null).
stdin:
	(you/should/not/see/this/error/1) 2> /dev/null
	you/should/not/see/this/error/2 2> /dev/null
	true
---
name: regression-15
description:
	The command
		$ whence foobar
	generates a blank line under pdksh and sets the exit status to 0.
	at&t ksh88 generates no output and sets the exit status to 1.  Also,
	the command
		$ whence foobar cat
	generates no output under at&t ksh88 (pdksh generates a blank line
	and /bin/cat).
stdin:
	whence does/not/exist > /dev/null
	echo 1: $?
	echo 2: $(whence does/not/exist | wc -l)
	echo 3: $(whence does/not/exist cat | wc -l)
expected-stdout:
	1: 1
	2: 0
	3: 0
---
name: regression-16
description:
	${var%%expr} seems to be broken in many places.  On the mips
	the commands
		$ read line < /etc/passwd
		$ echo $line
		root:0:1:...
		$ echo ${line%%:*}
		root
		$ echo $line
		root
		$
	change the value of line.  On sun4s & pas, the echo ${line%%:*} doesn't
	work.  Haven't checked elsewhere...
script:
	read x
	y=$x
	echo ${x%%:*}
	echo $x
stdin:
	root:asdjhasdasjhs:0:1:Root:/:/bin/sh
expected-stdout:
	root
	root:asdjhasdasjhs:0:1:Root:/:/bin/sh
---
name: regression-17
description:
	The command
		. /foo/bar
	should set the exit status to non-zero (sh and at&t ksh88 do).
	XXX doting a non existent file is a fatal error for a script
stdin:
	. does/not/exist
expected-exit: e != 0
expected-stderr-pattern: /.?/
---
name: regression-19
description:
	Both of the following echos should produce the same thing, but don't:
		$ x=foo/bar
		$ echo ${x%/*}
		foo
		$ echo "${x%/*}"
		foo/bar
stdin:
	x=foo/bar
	echo "${x%/*}"
expected-stdout:
	foo
---
name: regression-21
description:
	backslash does not work as expected in case labels:
	$ x='-x'
	$ case $x in
	-\?) echo hi
	esac
	hi
	$ x='-?'
	$ case $x in
	-\\?) echo hi
	esac
	hi
	$
stdin:
	case -x in
	-\?)	echo fail
	esac
---
name: regression-22
description:
	Quoting backquotes inside backquotes doesn't work:
	$ echo `echo hi \`echo there\` folks`
	asks for more info.  sh and at&t ksh88 both echo
	hi there folks
stdin:
	echo `echo hi \`echo there\` folks`
expected-stdout:
	hi there folks
---
name: regression-23
description:
	)) is not treated `correctly':
	    $ (echo hi ; (echo there ; echo folks))
	    missing ((
	    $
	instead of (as sh and ksh.att)
	    $ (echo hi ; (echo there ; echo folks))
	    hi
	    there
	    folks
	    $
stdin:
	( : ; ( : ; echo hi))
expected-stdout:
	hi
---
name: regression-25
description:
	Check reading stdin in a while loop.  The read should only read
	a single line, not a whole stdio buffer; the cat should get
	the rest.
stdin:
	(echo a; echo b) | while read x ; do
	    echo $x
	    cat > /dev/null
	done
expected-stdout:
	a
---
name: regression-26
description:
	Check reading stdin in a while loop.  The read should read both
	lines, not just the first.
script:
	a=
	while [ "$a" != xxx ] ; do
	    last=$x
	    read x
	    cat /dev/null | sed 's/x/y/'
	    a=x$a
	done
	echo $last
stdin:
	a
	b
expected-stdout:
	b
---
name: regression-27
description:
	The command
		. /does/not/exist
	should cause a script to exit.
stdin:
	. does/not/exist
	echo hi
expected-exit: e != 0
expected-stderr-pattern: /does\/not\/exist/
---
name: regression-28
description:
	variable assignements not detected well
stdin:
	a.x=1 echo hi
expected-exit: e != 0
expected-stderr-pattern: /a\.x=1/
---
name: regression-29
description:
	alias expansion different from at&t ksh88
stdin:
	alias a='for ' b='i in'
	a b hi ; do echo $i ; done
expected-stdout:
	hi
---
name: regression-30
description:
	strange characters allowed inside ${...}
stdin:
	echo ${a{b}}
expected-exit: e != 0
expected-stderr-pattern: /.?/
---
name: regression-31
description:
	Does read handle partial lines correctly
script:
	a= ret=
	while [ "$a" != xxx ] ; do
	    read x y z
	    ret=$?
	    a=x$a
	done
	echo "[$x]"
	echo $ret
stdin: !
	a A aA
	b B Bb
	c
expected-stdout:
	[c]
	1
---
name: regression-32
description:
	Does read set variables to null at eof?
script:
	a=
	while [ "$a" != xxx ] ; do
	    read x y z
	    a=x$a
	done
	echo 1: ${x-x not set} ${y-y not set} ${z-z not set}
	echo 2: ${x:+x not null} ${y:+y not null} ${z:+z not null}
stdin:
	a A Aa
	b B Bb
expected-stdout:
	1:
	2:
---
name: regression-33
description:
	Does umask print a leading 0 when umask is 3 digits?
stdin:
	umask 222
	umask
expected-stdout:
	0222
---
name: regression-35
description:
	Tempory files used for here-docs in functions get trashed after
	the function is parsed (before it is executed)
stdin:
	f1() {
		cat <<- EOF
			F1
		EOF
		f2() {
			cat <<- EOF
				F2
			EOF
		}
	}
	f1
	f2
	unset -f f1
	f2
expected-stdout:
	F1
	F2
	F2
---
name: regression-36
description:
	Command substitution breaks reading in while loop
	(test from <sjg@void.zen.oz.au>)
stdin:
	(echo abcdef; echo; echo 123) |
	    while read line
	    do
	      # the following line breaks it
	      c=`echo $line | wc -c`
	      echo $c
	    done
expected-stdout:
	7
	1
	4
---
name: regression-37
description:
	Machines with broken times() (reported by <sjg@void.zen.oz.au>)
	time does not report correct real time
stdin:
	time sleep 1
expected-stderr-pattern: !/^\s*0\.0[\s\d]+real|^\s*real[\s]+0+\.0/
---
name: regression-38
description:
	set -e doesn't ignore exit codes for if/while/until/&&/||/!.
arguments: !-e!
stdin:
	if false; then echo hi ; fi
	false || true
	false && true
	while false; do echo hi; done
	echo ok
expected-stdout:
	ok
---
name: regression-39
description:
	set -e: errors in command substitutions aren't ignored
	Not clear if they should be or not... bash passes here
	this may actually be required for make, so changed the
	test to make this an mksh feature, not a bug
arguments: !-e!
stdin:
	echo `false; echo hi`
#expected-fail: yes
#expected-stdout:
#	hi
expected-fail: no
expected-stdout:
	
---
name: regression-40
description:
	This used to cause a core dump
env-setup: !RANDOM=12!
stdin:
	echo hi
expected-stdout:
	hi
---
name: regression-41
description:
	foo should be set to bar (should not be empty)
stdin:
	foo=`
	echo bar`
	echo "($foo)"
expected-stdout:
	(bar)
---
name: regression-42
description:
	Can't use command line assignments to assign readonly parameters.
stdin:
	foo=bar
	readonly foo
	foo=stuff env | grep '^foo'
expected-exit: e != 0
expected-stderr-pattern:
	/.*read *only.*/
---
name: regression-43
description:
	Can subshells be prefixed by redirections (historical shells allow
	this)
stdin:
	< /dev/null (sed 's/^/X/')
---
name: regression-44
description:
	getopts sets OPTIND correctly for unparsed option
stdin:
	set -- -a -a -x
	while getopts :a optc; do
	    echo "OPTARG=$OPTARG, OPTIND=$OPTIND, optc=$optc."
	done
	echo done
expected-stdout:
	OPTARG=, OPTIND=2, optc=a.
	OPTARG=, OPTIND=3, optc=a.
	OPTARG=x, OPTIND=3, optc=?.
	done
---
name: regression-45
description:
	Parameter assignments with [] recognised correctly
stdin:
	FOO=*[12]
	BAR=abc[
	MORE=[abc]
	JUNK=a[bc
	echo "<$FOO>"
	echo "<$BAR>"
	echo "<$MORE>"
	echo "<$JUNK>"
expected-stdout:
	<*[12]>
	<abc[>
	<[abc]>
	<a[bc>
---
name: regression-46
description:
	Check that alias expansion works in command substitutions and
	at the end of file.
stdin:
	alias x='echo hi'
	FOO="`x` "
	echo "[$FOO]"
	x
expected-stdout:
	[hi ]
	hi
---
name: regression-47
description:
	Check that aliases are fully read.
stdin:
	alias x='echo hi;
	echo there'
	x
	echo done
expected-stdout:
	hi
	there
	done
---
name: regression-48
description:
	Check that (here doc) temp files are not left behind after an exec.
stdin:
	mkdir foo || exit 1
	TMPDIR=$PWD/foo "$__progname" <<- 'EOF'
		x() {
			sed 's/^/X /' << E_O_F
			hi
			there
			folks
			E_O_F
			echo "done ($?)"
		}
		echo=echo; [ -x /bin/echo ] && echo=/bin/echo
		exec $echo subtest-1 hi
	EOF
	echo subtest-1 foo/*
	TMPDIR=$PWD/foo "$__progname" <<- 'EOF'
		echo=echo; [ -x /bin/echo ] && echo=/bin/echo
		sed 's/^/X /' << E_O_F; exec $echo subtest-2 hi
		a
		few
		lines
		E_O_F
	EOF
	echo subtest-2 foo/*
expected-stdout:
	subtest-1 hi
	subtest-1 foo/*
	X a
	X few
	X lines
	subtest-2 hi
	subtest-2 foo/*
---
name: regression-49
description:
	Check that unset params with attributes are reported by set, those
	sans attributes are not.
stdin:
	unset FOO BAR
	echo X$FOO
	export BAR
	typeset -i BLAH
	set | grep FOO
	set | grep BAR
	set | grep BLAH
expected-stdout:
	X
	BAR
	BLAH
---
name: regression-50
description:
	Check that aliases do not use continuation prompt after trailing
	semi-colon.
file-setup: file 644 "env"
	PS1=Y
	PS2=X
env-setup: !ENV=./env!
arguments: !-i!
stdin:
	alias foo='echo hi ; '
	foo
	foo echo there
expected-stdout:
	hi
	hi
	there
expected-stderr: !
	YYYY
---
name: regression-51
description:
	Check that set allows both +o and -o options on same command line.
stdin:
	set a b c
	set -o noglob +o allexport
	echo A: $*, *
expected-stdout:
	A: a b c, *
---
name: regression-52
description:
	Check that globbing works in pipelined commands
file-setup: file 644 "env"
	PS1=P
file-setup: file 644 "abc"
	stuff
env-setup: !ENV=./env!
arguments: !-i!
stdin:
	sed 's/^/X /' < ab*
	echo mark 1
	sed 's/^/X /' < ab* | sed 's/^/Y /'
	echo mark 2
expected-stdout:
	X stuff
	mark 1
	Y X stuff
	mark 2
expected-stderr: !
	PPPPP
---
name: regression-53
description:
	Check that getopts works in functions
stdin:
	bfunc() {
	    echo bfunc: enter "(args: $*; OPTIND=$OPTIND)"
	    while getopts B oc; do
		case $oc in
		  (B)
		    echo bfunc: B option
		    ;;
		  (*)
		    echo bfunc: odd option "($oc)"
		    ;;
		esac
	    done
	    echo bfunc: leave
	}
	
	function kfunc {
	    echo kfunc: enter "(args: $*; OPTIND=$OPTIND)"
	    while getopts K oc; do
		case $oc in
		  (K)
		    echo kfunc: K option
		    ;;
		  (*)
		    echo bfunc: odd option "($oc)"
		    ;;
		esac
	    done
	    echo kfunc: leave
	}
	
	set -- -f -b -k -l
	echo "line 1: OPTIND=$OPTIND"
	getopts kbfl optc
	echo "line 2: ret=$?, optc=$optc, OPTIND=$OPTIND"
	bfunc -BBB blah
	echo "line 3: OPTIND=$OPTIND"
	getopts kbfl optc
	echo "line 4: ret=$?, optc=$optc, OPTIND=$OPTIND"
	kfunc -KKK blah
	echo "line 5: OPTIND=$OPTIND"
	getopts kbfl optc
	echo "line 6: ret=$?, optc=$optc, OPTIND=$OPTIND"
	echo
	
	OPTIND=1
	set -- -fbkl
	echo "line 10: OPTIND=$OPTIND"
	getopts kbfl optc
	echo "line 20: ret=$?, optc=$optc, OPTIND=$OPTIND"
	bfunc -BBB blah
	echo "line 30: OPTIND=$OPTIND"
	getopts kbfl optc
	echo "line 40: ret=$?, optc=$optc, OPTIND=$OPTIND"
	kfunc -KKK blah
	echo "line 50: OPTIND=$OPTIND"
	getopts kbfl optc
	echo "line 60: ret=$?, optc=$optc, OPTIND=$OPTIND"
expected-stdout:
	line 1: OPTIND=1
	line 2: ret=0, optc=f, OPTIND=2
	bfunc: enter (args: -BBB blah; OPTIND=2)
	bfunc: B option
	bfunc: B option
	bfunc: leave
	line 3: OPTIND=2
	line 4: ret=0, optc=b, OPTIND=3
	kfunc: enter (args: -KKK blah; OPTIND=1)
	kfunc: K option
	kfunc: K option
	kfunc: K option
	kfunc: leave
	line 5: OPTIND=3
	line 6: ret=0, optc=k, OPTIND=4
	
	line 10: OPTIND=1
	line 20: ret=0, optc=f, OPTIND=2
	bfunc: enter (args: -BBB blah; OPTIND=2)
	bfunc: B option
	bfunc: B option
	bfunc: leave
	line 30: OPTIND=2
	line 40: ret=1, optc=?, OPTIND=2
	kfunc: enter (args: -KKK blah; OPTIND=1)
	kfunc: K option
	kfunc: K option
	kfunc: K option
	kfunc: leave
	line 50: OPTIND=2
	line 60: ret=1, optc=?, OPTIND=2
---
name: regression-54
description:
	Check that ; is not required before the then in if (( ... )) then ...
stdin:
	if (( 1 )) then
	    echo ok dparen
	fi
	if [[ -n 1 ]] then
	    echo ok dbrackets
	fi
expected-stdout:
	ok dparen
	ok dbrackets
---
name: regression-55
description:
	Check ${foo:%bar} is allowed (ksh88 allows it...)
stdin:
	x=fooXbarXblah
	echo 1 ${x%X*}
	echo 2 ${x:%X*}
	echo 3 ${x%%X*}
	echo 4 ${x:%%X*}
	echo 5 ${x#*X}
	echo 6 ${x:#*X}
	echo 7 ${x##*X}
	echo 8 ${x:##*X}
expected-stdout:
	1 fooXbar
	2 fooXbar
	3 foo
	4 foo
	5 barXblah
	6 barXblah
	7 blah
	8 blah
---
name: regression-56
description:
	Check eval vs substitution exit codes
	(this is what ksh88 does)
stdin:
	eval $(false)
	echo A $?
	eval ' $(false)'
	echo B $?
	eval " $(false)"
	echo C $?
	eval "eval $(false)"
	echo D $?
	eval 'eval '"$(false)"
	echo E $?
	IFS="$IFS:"
	eval $(echo :; false)
	echo F $?
expected-stdout:
	A 1
	B 1
	C 1
	D 0
	E 0
	F 1
---
name: regression-57
description:
	Check if typeset output is correct for
	uninitialised array elements.
stdin:
	typeset -i xxx[4]
	echo A
	typeset -i | grep xxx | sed 's/^/    /'
	echo B
	typeset | grep xxx | sed 's/^/    /'
	
	xxx[1]=2+5
	echo M
	typeset -i | grep xxx | sed 's/^/    /'
	echo N
	typeset | grep xxx | sed 's/^/    /'
expected-stdout:
	A
	    xxx
	B
	    typeset -i xxx
	M
	    xxx[1]=7
	N
	    typeset -i xxx
---
name: regression-58
description:
	Check if trap exit is ok (exit not mistaken for signal name)
stdin:
	trap 'echo hi' exit
	trap exit 1
expected-stdout:
	hi
---
name: regression-59
description:
	Check if ${#array[*]} is calculated correctly.
stdin:
	a[12]=hi
	a[8]=there
	echo ${#a[*]}
expected-stdout:
	2
---
name: regression-60
description:
	Check if default exit status is previous command
stdin:
	(true; exit)
	echo A $?
	(false; exit)
	echo B $?
	( (exit 103) ; exit)
	echo C $?
expected-stdout:
	A 0
	B 1
	C 103
---
name: regression-61
description:
	Check if EXIT trap is executed for sub shells.
stdin:
	trap 'echo parent exit' EXIT
	echo start
	(echo A; echo A last)
	echo B
	(echo C; trap 'echo sub exit' EXIT; echo C last)
	echo parent last
expected-stdout:
	start
	A
	A last
	B
	C
	C last
	sub exit
	parent last
	parent exit
---
name: regression-62
description:
	Check if test -nt/-ot succeeds if second(first) file is missing.
stdin:
	touch a
	test a -nt b && echo nt OK || echo nt BAD
	test b -ot a && echo ot OK || echo ot BAD
expected-stdout:
	nt OK
	ot OK
---
name: regression-63
description:
	Check if typeset, export, and readonly work
stdin:
	{
		print FNORD-0
		FNORD_A=1
		FNORD_B=2
		FNORD_C=3
		FNORD_D=4
		FNORD_E=5
		FNORD_F=6
		FNORD_G=7
		FNORD_H=8
		integer FNORD_E FNORD_F FNORD_G FNORD_H
		export FNORD_C FNORD_D FNORD_G FNORD_H
		readonly FNORD_B FNORD_D FNORD_F FNORD_H
		print FNORD-1
		export
		print FNORD-2
		export -p
		print FNORD-3
		readonly
		print FNORD-4
		readonly -p
		print FNORD-5
		typeset
		print FNORD-6
		typeset -p
		print FNORD-7
		typeset -
		print FNORD-8
	} | fgrep FNORD
expected-stdout:
	FNORD-0
	FNORD-1
	FNORD_C
	FNORD_D
	FNORD_G
	FNORD_H
	FNORD-2
	export FNORD_C=3
	export FNORD_D=4
	export FNORD_G=7
	export FNORD_H=8
	FNORD-3
	FNORD_B
	FNORD_D
	FNORD_F
	FNORD_H
	FNORD-4
	readonly FNORD_B=2
	readonly FNORD_D=4
	readonly FNORD_F=6
	readonly FNORD_H=8
	FNORD-5
	typeset FNORD_A
	typeset -r FNORD_B
	typeset -x FNORD_C
	typeset -x -r FNORD_D
	typeset -i FNORD_E
	typeset -i -r FNORD_F
	typeset -i -x FNORD_G
	typeset -i -x -r FNORD_H
	FNORD-6
	typeset FNORD_A=1
	typeset -r FNORD_B=2
	typeset -x FNORD_C=3
	typeset -x -r FNORD_D=4
	typeset -i FNORD_E=5
	typeset -i -r FNORD_F=6
	typeset -i -x FNORD_G=7
	typeset -i -x -r FNORD_H=8
	FNORD-7
	FNORD_A=1
	FNORD_B=2
	FNORD_C=3
	FNORD_D=4
	FNORD_E=5
	FNORD_F=6
	FNORD_G=7
	FNORD_H=8
	FNORD-8
---
name: regression-64
description:
	Check that we can redefine functions calling time builtin
stdin:
	t() {
		time >/dev/null
	}
	t 2>/dev/null
	t() {
		time
	}
---
name: syntax-1
description:
	Check that lone ampersand is a syntax error
stdin:
	 &
expected-exit: e != 0
expected-stderr-pattern:
	/syntax error/
---
name: xxx-quoted-newline-1
description:
	Check that \<newline> works inside of ${}
stdin:
	abc=2
	echo ${ab\
	c}
expected-stdout:
	2
---
name: xxx-quoted-newline-2
description:
	Check that \<newline> works at the start of a here document
stdin:
	cat << EO\
	F
	hi
	EOF
expected-stdout:
	hi
---
name: xxx-quoted-newline-3
description:
	Check that \<newline> works at the end of a here document
stdin:
	cat << EOF
	hi
	EO\
	F
expected-stdout:
	hi
---
name: xxx-multi-assignment-cmd
description:
	Check that assignments in a command affect subsequent assignments
	in the same command
stdin:
	FOO=abc
	FOO=123 BAR=$FOO
	echo $BAR
expected-stdout:
	123
---
name: xxx-exec-environment-1
description:
	Check to see if exec sets it's environment correctly
stdin:
	FOO=bar exec env
expected-stdout-pattern:
	/(^|.*\n)FOO=bar\n/
---
name: xxx-exec-environment-2
description:
	Check to make sure exec doesn't change environment if a program
	isn't exec-ed
stdin:
	env > bar1
	FOO=bar exec; env > bar2
	cmp -s bar1 bar2
---
name: xxx-what-do-you-call-this-1
stdin:
	echo "${foo:-"a"}*"
expected-stdout:
	a*
---
name: xxx-prefix-strip-1
stdin:
	foo='a cdef'
	echo ${foo#a c}
expected-stdout:
	def
---
name: xxx-prefix-strip-2
stdin:
	set a c
	x='a cdef'
	echo ${x#$*}
expected-stdout:
	def
---
name: xxx-variable-syntax-1
stdin:
	echo ${:}
expected-stderr-pattern:
	/bad substitution/
expected-exit: 1
---
name: xxx-substitution-eval-order
description:
	Check order of evaluation of expressions
stdin:
	i=1 x= y=
	set -A A abc def GHI j G k
	echo ${A[x=(i+=1)]#${A[y=(i+=2)]}}
	echo $x $y
expected-stdout:
	HI
	2 4
---
name: xxx-set-option-1
description:
	Check option parsing in set
stdin:
	set -vsA foo -- A 1 3 2
	echo ${foo[*]}
expected-stderr:
	echo ${foo[*]}
expected-stdout:
	1 2 3 A
---
name: xxx-exec-1
description:
	Check that exec exits for built-ins
arguments: !-i!
stdin:
	exec print hi
	echo still herre
expected-stdout:
	hi
expected-stderr-pattern: /.*/
---
name: xxx-while-1
description:
	Check the return value of while loops
	XXX need to do same for for/select/until loops
stdin:
	i=x
	while [ $i != xxx ] ; do
	    i=x$i
	    if [ $i = xxx ] ; then
		false
		continue
	    fi
	done
	echo loop1=$?
	
	i=x
	while [ $i != xxx ] ; do
	    i=x$i
	    if [ $i = xxx ] ; then
		false
		break
	    fi
	done
	echo loop2=$?
	
	i=x
	while [ $i != xxx ] ; do
	    i=x$i
	    false
	done
	echo loop3=$?
expected-stdout:
	loop1=0
	loop2=0
	loop3=1
---
name: xxx-status-1
description:
	Check that blank lines don't clear $?
arguments: !-i!
stdin:
	(exit 1)
	echo $?
	(exit 1)
	
	echo $?
	true
expected-stdout:
	1
	1
expected-stderr-pattern: /.*/
---
name: xxx-status-2
description:
	Check that $? is preserved in subshells, includes, traps.
stdin:
	(exit 1)
	
	echo blank: $?
	
	(exit 2)
	(echo subshell: $?)
	
	echo 'echo include: $?' > foo
	(exit 3)
	. ./foo
	
	trap 'echo trap: $?' ERR
	(exit 4)
	echo exit: $?
expected-stdout:
	blank: 1
	subshell: 2
	include: 3
	trap: 4
	exit: 4
---
name: xxx-clean-chars-1
description:
	Check MAGIC character is stuffed correctly
stdin:
	echo `echo [�`
expected-stdout:
	[�
---
name: xxx-param-subst-qmark-1
description:
	Check suppresion of error message with null string.  According to
	POSIX, it shouldn't print the error as 'word' isn't ommitted.
	ksh88, Solaris /bin/sh and /usr/xpg4/bin/sh all print the error,
	that's why the condition is reversed.
stdin:
	unset foo
	x=
	echo x${foo?$x}
expected-exit: 1
# POSIX
#expected-fail: yes
#expected-stderr-pattern: !/not set/
# common use
expected-stderr-pattern: /parameter null or not set/
---
name: xxx-param-_-1
# fails due to weirdness of execv stuff
category: !os:uwin-nt
description:
	Check c flag is set.
arguments: !-c!echo "[$-]"!
expected-stdout-pattern: /^\[.*c.*\]$/
---
name: tilde-expand-1
description:
	Check tilde expansion after equal signs
env-setup: !HOME=/sweet!
stdin:
	echo ${A=a=}~ b=~ c=d~ ~
	set +o braceexpand
	echo ${A=a=}~ b=~ c=d~ ~
expected-stdout:
	a=/sweet b=/sweet c=d~ /sweet
	a=~ b=~ c=d~ /sweet
---
name: errexit-1
description:
	Check some "exit on error" conditions
stdin:
	set -ex
	/usr/bin/env false && echo something
	echo END
expected-stdout:
	END
expected-stderr:
	+ /usr/bin/env false
	+ echo END
---
name: errexit-2
description:
	Check some "exit on error" edge conditions needed for make(1)
stdin:
	set -ex
	if /usr/bin/env true; then
		/usr/bin/env false && echo something
	fi
	echo END
expected-stdout:
expected-stderr:
	+ /usr/bin/env true
	+ /usr/bin/env false
expected-exit: e != 0
---
name: errexit-3
description:
	pdksh regression which AT&T ksh does right
	TFM says: [set] -e | errexit
		Exit (after executing the ERR trap) ...
stdin:
	trap 'echo EXIT' EXIT
	trap 'echo ERR' ERR
	set -e
	cd /XXXXX 2>/dev/null
	echo DONE
	exit 0
expected-stdout:
	ERR
	EXIT
expected-exit: e != 0
---
name: test-stlt-1
description:
	Check that test also can handle string1 < string2 etc.
stdin:
	test 2005/10/08 '<' 2005/08/21 && echo ja || echo nein
	test 2005/08/21 \< 2005/10/08 && echo ja || echo nein
	test 2005/10/08 '>' 2005/08/21 && echo ja || echo nein
	test 2005/08/21 \> 2005/10/08 && echo ja || echo nein
expected-stdout:
	nein
	ja
	ja
	nein
expected-stderr-pattern: !/unexpected op/
---
name: mkshrc-1
description:
	Check that ~/.mkshrc works correctly.
	Part 1: verify user environment is not read (internal)
stdin:
	echo x $FNORD
expected-stdout:
	x
---
name: mkshrc-2a
description:
	Check that ~/.mkshrc works correctly.
	Part 2: verify mkshrc is not read (non-interactive shells)
file-setup: file 644 ".mkshrc"
	FNORD=42
env-setup: !HOME=.!ENV=!
stdin:
	echo x $FNORD
expected-stdout:
	x
---
name: mkshrc-2b
description:
	Check that ~/.mkshrc works correctly.
	Part 2: verify mkshrc can be read (interactive shells)
file-setup: file 644 ".mkshrc"
	FNORD=42
arguments: !-i!
env-setup: !HOME=.!ENV=!PS1=!
stdin:
	echo x $FNORD
expected-stdout:
	x 42
expected-stderr-pattern:
	/(# )*/
---
name: mkshrc-3
description:
	Check that ~/.mkshrc works correctly.
	Part 3: verify mkshrc can be turned off
file-setup: file 644 ".mkshrc"
	FNORD=42
env-setup: !HOME=.!ENV=nonexistant!
stdin:
	echo x $FNORD
expected-stdout:
	x
---
name: posix-mode-1
description:
	Check that posix mode turns braceexpand off
	and that that works correctly
stdin:
	set -o braceexpand
	set +o posix
	[[ $(set +o) == *@(-o posix)@(| *) ]] && echo posix || echo noposix
	[[ $(set +o) == *@(-o braceexpand)@(| *) ]] && echo brex || echo nobrex
	echo {a,b,c}
	set +o braceexpand
	echo {a,b,c}
	set -o braceexpand
	echo {a,b,c}
	set -o posix
	echo {a,b,c}
	[[ $(set +o) == *@(-o posix)@(| *) ]] && echo posix || echo noposix
	[[ $(set +o) == *@(-o braceexpand)@(| *) ]] && echo brex || echo nobrex
	set -o braceexpand
	echo {a,b,c}
	[[ $(set +o) == *@(-o posix)@(| *) ]] && echo posix || echo noposix
	[[ $(set +o) == *@(-o braceexpand)@(| *) ]] && echo brex || echo nobrex
expected-stdout:
	noposix
	brex
	a b c
	{a,b,c}
	a b c
	{a,b,c}
	posix
	nobrex
	a b c
	posix
	brex
---
name: posix-mode-2a
description:
	Check that posix mode is *not* automatically turned on
category: !binsh
stdin:
	ln -s "$__progname" ksh
	ln -s "$__progname" sh
	ln -s "$__progname" ./-ksh
	ln -s "$__progname" ./-sh
	for shell in {,-}{,k}sh; do
		print -- $shell $(./$shell +l -c \
		    '[[ $(set +o) == *@(-o posix)@(| *) ]] && echo posix || echo noposix')
	done
expected-stdout:
	sh noposix
	ksh noposix
	-sh noposix
	-ksh noposix
---
name: posix-mode-2b
description:
	Check that posix mode is automatically turned on
category: binsh
stdin:
	ln -s "$__progname" ksh
	ln -s "$__progname" sh
	ln -s "$__progname" ./-ksh
	ln -s "$__progname" ./-sh
	for shell in {,-}{,k}sh; do
		print -- $shell $(./$shell +l -c \
		    '[[ $(set +o) == *@(-o posix)@(| *) ]] && echo posix || echo noposix')
	done
expected-stdout:
	sh posix
	ksh noposix
	-sh posix
	-ksh noposix
---
name: pipeline-1
description:
	pdksh bug: last command of a pipeline is executed in a
	subshell - make sure it still is, scripts depend on it
file-setup: file 644 "abcx"
file-setup: file 644 "abcy"
stdin:
	echo *
	echo a | while read d; do
		echo $d
		echo $d*
		echo *
		set -o noglob
		echo $d*
		echo *
	done
	echo *
expected-stdout:
	abcx abcy
	a
	abcx abcy
	abcx abcy
	a*
	*
	abcx abcy
---
name: pipeline-2
description:
	check that co-processes work with TCOMs, TPIPEs and TPARENs
stdin:
	"$__progname" -c 'i=100; print hi |& while read -p line; do print "$((i++)) $line"; done'
	"$__progname" -c 'i=200; print hi | cat |& while read -p line; do print "$((i++)) $line"; done'
	"$__progname" -c 'i=300; (print hi | cat) |& while read -p line; do print "$((i++)) $line"; done'
expected-stdout:
	100 hi
	200 hi
	300 hi
---
name: persist-history-1
description:
	Check if persistent history saving works
category: !no-histfile
arguments: !-i!
env-setup: !ENV=./Env!HISTFILE=hist.file!
file-setup: file 644 "Env"
	PS1=X
stdin:
	cat hist.file
expected-stdout-pattern:
	/cat hist.file/
expected-stderr-pattern:
	/^X*$/
---
name: typeset-padding-1
description:
	Check if left/right justification works as per TFM
stdin:
	typeset -L10 ln=0hall0
	typeset -R10 rn=0hall0
	typeset -ZL10 lz=0hall0
	typeset -ZR10 rz=0hall0
	typeset -Z10 rx=" hallo "
	print "<$ln> <$rn> <$lz> <$rz> <$rx>"
expected-stdout:
	<0hall0    > <    0hall0> <hall0     > <00000hall0> <0000 hallo>
---
name: typeset-padding-2
description:
	Check if base-!10 integers are padded right
stdin:
	typeset -Uui16 -L9 ln=16#1
	typeset -Uui16 -R9 rn=16#1
	typeset -Uui16 -Z9 zn=16#1
	typeset -L9 ls=16#1
	typeset -R9 rs=16#1
	typeset -Z9 zs=16#1
	print "<$ln> <$rn> <$zn> <$ls> <$rs> <$zs>"
expected-stdout:
	<16#1     > <     16#1> <16#000001> <16#1     > <     16#1> <0000016#1>
---
name: utf8bom-1
description:
	Check that the UTF-8 Byte Order Mark is ignored as the first
	multibyte character of the shell input (with -c, from standard
	input, as file, or as eval argument), but nowhere else
# breaks on Mac OSX (HFS+ non-standard Unicode canonical decomposition)
category: !os:darwin
stdin:
	mkdir foo
	print '#!/bin/sh\necho ohne' >foo/fnord
	print '#!/bin/sh\necho mit' >foo/﻿fnord
	print '﻿fnord\nfnord\n﻿fnord\nfnord' >foo/bar
	print eval \''﻿fnord\nfnord\n﻿fnord\nfnord'\' >foo/zoo
	set -A anzahl -- foo/*
	print got ${#anzahl[*]} files
	chmod +x foo/*
	export PATH=$(pwd)/foo:$PATH
	"$__progname" -c '﻿fnord'
	"$__progname" -c '﻿fnord; fnord; ﻿fnord; fnord'
	"$__progname" foo/bar
	"$__progname" <foo/bar
	"$__progname" foo/zoo
	"$__progname" -c 'print ﻿: $(﻿fnord)'
	rm -rf foo
expected-stdout:
	got 4 files
	ohne
	ohne
	ohne
	mit
	ohne
	ohne
	ohne
	mit
	ohne
	ohne
	ohne
	mit
	ohne
	ohne
	ohne
	mit
	ohne
	﻿: mit
---
name: utf8bom-2
description:
	Check that we can execute BOM-shebangs
	XXX if the OS can already execute them, we lose
	note: cygwin execve(2) doesn't return to us with ENOEXEC, we lose
	note: Ultrix perl5 t4 returns 65280 (exit-code 255) and no text
category: !os:cygwin,!os:uwin-nt,!os:ultrix,!smksh
env-setup: !FOO=BAR!
stdin:
	print '#!'"$__progname"'\nprint "a=$ENV{FOO}";' >t1
	print '﻿#!'"$__progname"'\nprint "a=$ENV{FOO}";' >t2
	print '#!'"$__perlname"'\nprint "a=$ENV{FOO}\n";' >t3
	print '﻿#!'"$__perlname"'\nprint "a=$ENV{FOO}\n";' >t4
	chmod +x t?
	./t1
	./t2
	./t3
	./t4
expected-stdout:
	a=/nonexistant{FOO}
	a=/nonexistant{FOO}
	a=BAR
	a=BAR
expected-stderr-pattern:
	/(Unrecognized character .... ignored at \..t4 line 1)*/
---
name: utf8bom-3
description:
	Reading the UTF-8 BOM should enable the utf8-mode flag
stdin:
	"$__progname" -c ':; if [[ $- = *U* ]]; then print on; else print off; fi'
	"$__progname" -c '﻿:; if [[ $- = *U* ]]; then print on; else print off; fi'
expected-stdout:
	off
	on
---
name: utf8opt-1a
description:
	Check that the utf8-mode flag is not set at non-interactive startup
category: !os:hpux
env-setup: !PS1=!PS2=!LC_CTYPE=en_US.UTF-8!
stdin:
	if [[ $- = *U* ]]; then
		print is set
	else
		print is not set
	fi
expected-stdout:
	is not set
---
name: utf8opt-1b
description:
	Check that the utf8-mode flag is not set at non-interactive startup
category: os:hpux
env-setup: !PS1=!PS2=!LC_CTYPE=en_US.utf8!
stdin:
	if [[ $- = *U* ]]; then
		print is set
	else
		print is not set
	fi
expected-stdout:
	is not set
---
name: utf8opt-2a
description:
	Check that the utf8-mode flag is set at interactive startup
category: !os:hpux
arguments: !-i!
env-setup: !PS1=!PS2=!LC_CTYPE=en_US.UTF-8!
stdin:
	if [[ $- = *U* ]]; then
		print is set
	else
		print is not set
	fi
expected-stdout:
	is set
expected-stderr-pattern:
	/(# )*/
---
name: utf8opt-2b
description:
	Check that the utf8-mode flag is set at interactive startup
category: os:hpux
arguments: !-i!
env-setup: !PS1=!PS2=!LC_CTYPE=en_US.utf8!
stdin:
	if [[ $- = *U* ]]; then
		print is set
	else
		print is not set
	fi
expected-stdout:
	is set
expected-stderr-pattern:
	/(# )*/
---
name: aliases-1
description:
	Check if built-in shell aliases are okay
stdin:
	alias
	typeset -f
expected-stdout:
	autoload='typeset -fu'
	functions='typeset -f'
	hash='alias -t'
	history='fc -l'
	integer='typeset -i'
	local=typeset
	login='exec login'
	nohup='nohup '
	r='fc -e -'
	source='PATH=$PATH:. command .'
	stop='kill -STOP'
	suspend='kill -STOP $$'
	type='whence -v'
---
name: aliases-2a
description:
	Check if “set -o posix” disables built-in aliases (except a few)
category: disabled
arguments: !-o!posix!
stdin:
	alias
	typeset -f
expected-stdout:
	integer='typeset -i'
	local=typeset
---
name: aliases-3a
description:
	Check if running as sh disables built-in aliases (except a few)
category: disabled
arguments: !-o!posix!
stdin:
	cp "$__progname" sh
	./sh -c 'alias; typeset -f'
	rm -f sh
expected-stdout:
	integer='typeset -i'
	local=typeset
---
name: aliases-2b
description:
	Check if “set -o posix” does not influence built-in aliases
arguments: !-o!posix!
stdin:
	alias
	typeset -f
expected-stdout:
	autoload='typeset -fu'
	functions='typeset -f'
	hash='alias -t'
	history='fc -l'
	integer='typeset -i'
	local=typeset
	login='exec login'
	nohup='nohup '
	r='fc -e -'
	source='PATH=$PATH:. command .'
	stop='kill -STOP'
	suspend='kill -STOP $$'
	type='whence -v'
---
name: aliases-3b
description:
	Check if running as sh does not influence built-in aliases
arguments: !-o!posix!
stdin:
	cp "$__progname" sh
	./sh -c 'alias; typeset -f'
	rm -f sh
expected-stdout:
	autoload='typeset -fu'
	functions='typeset -f'
	hash='alias -t'
	history='fc -l'
	integer='typeset -i'
	local=typeset
	login='exec login'
	nohup='nohup '
	r='fc -e -'
	source='PATH=$PATH:. command .'
	stop='kill -STOP'
	suspend='kill -STOP $$'
	type='whence -v'
---
name: arrays-1
description:
	Check if Korn Shell arrays work as expected
stdin:
	v="c d"
	set -A foo -- a \$v "$v" '$v' b
	echo "${#foo[*]}|${foo[0]}|${foo[1]}|${foo[2]}|${foo[3]}|${foo[4]}|"
expected-stdout:
	5|a|$v|c d|$v|b|
---
name: arrays-2
description:
	Check if bash-style arrays work as expected
stdin:
	v="c d"
	foo=(a \$v "$v" '$v' b)
	echo "${#foo[*]}|${foo[0]}|${foo[1]}|${foo[2]}|${foo[3]}|${foo[4]}|"
expected-stdout:
	5|a|$v|c d|$v|b|
---
name: arrays-3
description:
	Check if array bounds are uint32_t
stdin:
	set -A foo a b c
	foo[4097]=d
	foo[2147483637]=e
	print ${foo[*]}
	foo[-1]=f
	print ${foo[4294967295]} g ${foo[*]}
expected-stdout:
	a b c d e
	f g a b c d e f
---
name: varexpand-substr-1
description:
	Check if bash-style substring expansion works
	when using positive numerics
stdin:
	x=abcdefghi
	typeset -i y=123456789
	typeset -i 16 z=123456789	# 16#75bcd15
	print a t${x:2:2} ${y:2:3} ${z:2:3} a
	print b ${x::3} ${y::3} ${z::3} b
	print c ${x:2:} ${y:2:} ${z:2:} c
	print d ${x:2} ${y:2} ${z:2} d
	print e ${x:2:6} ${y:2:6} ${z:2:7} e
	print f ${x:2:7} ${y:2:7} ${z:2:8} f
	print g ${x:2:8} ${y:2:8} ${z:2:9} g
expected-stdout:
	a tcd 345 #75 a
	b abc 123 16# b
	c c
	d cdefghi 3456789 #75bcd15 d
	e cdefgh 345678 #75bcd1 e
	f cdefghi 3456789 #75bcd15 f
	g cdefghi 3456789 #75bcd15 g
---
name: varexpand-substr-2
description:
	Check if bash-style substring expansion works
	when using negative numerics or expressions
stdin:
	x=abcdefghi
	typeset -i y=123456789
	typeset -i 16 z=123456789	# 16#75bcd15
	n=2
	print a ${x:$n:3} ${y:$n:3} ${z:$n:3} a
	print b ${x:(n):3} ${y:(n):3} ${z:(n):3} b
	print c ${x:(-2):1} ${y:(-2):1} ${z:(-2):1} c
	print d t${x: n:2} ${y: n:3} ${z: n:3} d
expected-stdout:
	a cde 345 #75 a
	b cde 345 #75 b
	c h 8 1 c
	d tcd 345 #75 d
---
name: varexpand-substr-3
description:
	Check that some things that work in bash fail.
	This is by design. And that some things fail in both.
stdin:
	export x=abcdefghi n=2
	"$__progname" -c 'print v${x:(n)}x'
	"$__progname" -c 'print w${x: n}x'
	"$__progname" -c 'print x${x:n}x'
	"$__progname" -c 'print y${x:}x'
	"$__progname" -c 'print z${x}x'
	"$__progname" -c 'x=abcdef;y=123;echo ${x:${y:2:1}:2}' >/dev/null 2>&1; print $?
expected-stdout:
	vcdefghix
	wcdefghix
	zabcdefghix
	1
expected-stderr-pattern:
	/x:n.*bad substitution.*\n.*bad substitution/
---
name: varexpand-substr-4
description:
	Check corner cases for substring expansion
stdin:
	x=abcdefghi
	integer y=2
	print a ${x:(y == 1 ? 2 : 3):4} a
expected-stdout:
	a defg a
---
name: print-funny-chars
description:
	Check print builtin's capability to output designated characters
stdin:
	print '<\0144\0344\xDB\u00DB\u20AC\uDB\x40>'
expected-stdout:
	<d��Û€Û@>
---
name: print-nul-chars
description:
	Check handling of NUL characters for print and read
	note: second line should output “4 3” but we cannot
	handle NUL characters in strings yet
stdin:
	print $(($(print '<\0>' | wc -c)))
	x=$(print '<\0>')
	print $(($(print "$x" | wc -c))) ${#x}
expected-stdout:
	4
	3 2
---
name: dot-needs-argument
description:
	check Debian #415167 solution: '.' without arguments should fail
stdin:
	"$__progname" -c .
	"$__progname" -c source
expected-exit: e != 0
expected-stderr-pattern:
	/\.: missing argument.*\n.*\.: missing argument/
---
name: alias-function-no-conflict
description:
	make aliases not conflict with functions
	note: for ksh-like functions, the order of preference is
	different; bash outputs baz instead of bar in line 2 below
stdin:
	alias foo='echo bar'
	foo() {
		echo baz
	}
	alias korn='echo bar'
	function korn {
		echo baz
	}
	foo
	korn
	unset -f foo
	foo 2>&- || echo rab
expected-stdout:
	baz
	bar
	rab
---
name: integer-base-one-1
description:
	check if the use of fake integer base 1 works
stdin:
	set -U
	typeset -Uui16 i0=1#� i1=1#€
	typeset -i1 o0a=64
	typeset -i1 o1a=0x263A
	typeset -Uui1 o0b=0x7E
	typeset -Uui1 o1b=0xFDD0
	integer px=0xCAFE 'p0=1# ' p1=1#… pl=1#f
	print "in <$i0> <$i1>"
	print "out <${o0a#1#}|${o0b#1#}> <${o1a#1#}|${o1b#1#}>"
	typeset -Uui1 i0 i1
	print "pass <$px> <$p0> <$p1> <$pl> <${i0#1#}|${i1#1#}>"
	typeset -Uui16 tv1=1#~ tv2=1# tv3=1#� tv4=1#� tv5=1#� tv6=1#� tv7=1#  tv8=1#
	print "specX <${tv1#16#}> <${tv2#16#}> <${tv3#16#}> <${tv4#16#}> <${tv5#16#}> <${tv6#16#}> <${tv7#16#}> <${tv8#16#}>"
	typeset -i1 tv1 tv2 tv3 tv4 tv5 tv6 tv7 tv8
	print "specW <${tv1#1#}> <${tv2#1#}> <${tv3#1#}> <${tv4#1#}> <${tv5#1#}> <${tv6#1#}> <${tv7#1#}> <${tv8#1#}>"
	typeset -i1 xs1=0xEF7F xs2=0xEF80 xs3=0xFDD0
	print "specU <${xs1#1#}> <${xs2#1#}> <${xs3#1#}>"
expected-stdout:
	in <16#EFEF> <16#20AC>
	out <@|~> <☺|﷐>
	pass <16#cafe> <1# > <1#…> <1#f> <�|€>
	specX <7E> <7F> <EF80> <EF81> <EFC0> <EFC1> <A0> <80>
	specW <~> <> <�> <�> <�> <�> < > <>
	specU <> <�> <﷐>
---
name: integer-base-one-2a
description:
	check if the use of fake integer base 1 stops at correct characters
stdin:
	set -U
	integer x=1#foo
	print /$x/
expected-stderr-pattern:
	/1#foo: unexpected 'oo'/
expected-exit: e != 0
---
name: integer-base-one-2b
description:
	check if the use of fake integer base 1 stops at correct characters
stdin:
	set -U
	integer x=1#��
	print /$x/
expected-stderr-pattern:
	/1#��: unexpected '�'/
expected-exit: e != 0
---
name: integer-base-one-2c1
description:
	check if the use of fake integer base 1 stops at correct characters
stdin:
	set -U
	integer x=1#…
	print /$x/
expected-stdout:
	/1#…/
---
name: integer-base-one-2c2
description:
	check if the use of fake integer base 1 stops at correct characters
stdin:
	set +U
	integer x=1#…
	print /$x/
expected-stderr-pattern:
	/1#…: unexpected '�'/
expected-exit: e != 0
---
name: integer-base-one-2d1
description:
	check if the use of fake integer base 1 handles octets okay
stdin:
	set -U
	typeset -i16 x=1#�
	print /$x/	# invalid utf-8
expected-stdout:
	/16#efff/
---
name: integer-base-one-2d2
description:
	check if the use of fake integer base 1 handles octets
stdin:
	set -U
	typeset -i16 x=1#�
	print /$x/	# invalid 2-byte
expected-stdout:
	/16#efc2/
---
name: integer-base-one-2d3
description:
	check if the use of fake integer base 1 handles octets
stdin:
	set -U
	typeset -i16 x=1#�
	print /$x/	# invalid 2-byte
expected-stdout:
	/16#efef/
---
name: integer-base-one-2d4
description:
	check if the use of fake integer base 1 stops at invalid input
stdin:
	set -U
	typeset -i16 x=1#��
	print /$x/	# invalid 3-byte
expected-stderr-pattern:
	/1#��: unexpected '�'/
expected-exit: e != 0
---
name: integer-base-one-2d5
description:
	check if the use of fake integer base 1 stops at invalid input
stdin:
	set -U
	typeset -i16 x=1#��
	print /$x/	# non-minimalistic
expected-stderr-pattern:
	/1#��: unexpected '�'/
expected-exit: e != 0
---
name: integer-base-one-2d6
description:
	check if the use of fake integer base 1 stops at invalid input
stdin:
	set -U
	typeset -i16 x=1#���
	print /$x/	# non-minimalistic
expected-stderr-pattern:
	/1#���: unexpected '�'/
expected-exit: e != 0
---
name: integer-base-one-3a
description:
	some sample code for hexdumping
stdin:
	{
		print 'Hello, World!\\\nこんにちは！'
		typeset -Uui16 i=0x100
		# change that to 0xFF once we can handle embedded
		# NUL characters in strings / here documents
		while (( i++ < 0x1FF )); do
			print -n "\x${i#16#1}"
		done
		print
	} | {
		typeset -Uui16 -Z11 pos=0
		typeset -Uui16 -Z5 hv
		typeset -i1 wc=0x0A
		dasc=
		nl=${wc#1#}
		while IFS= read -r line; do
			line=$line$nl
			while [[ -n $line ]]; do
				hv=1#${line::1}
				if (( (pos & 15) == 0 )); then
					(( pos )) && print "$dasc|"
					print -n "${pos#16#}  "
					dasc=' |'
				fi
				print -n "${hv#16#} "
				if (( (hv < 32) || (hv > 126) )); then
					dasc=$dasc.
				else
					dasc=$dasc${line::1}
				fi
				(( (pos++ & 15) == 7 )) && print -n -- '- '
				line=${line:1}
			done
		done
		if (( (pos & 15) != 1 )); then
			while (( pos & 15 )); do
				print -n '   '
				(( (pos++ & 15) == 7 )) && print -n -- '- '
			done
			print "$dasc|"
		fi
	}
expected-stdout:
	00000000  48 65 6C 6C 6F 2C 20 57 - 6F 72 6C 64 21 5C 0A E3  |Hello, World!\..|
	00000010  81 93 E3 82 93 E3 81 AB - E3 81 A1 E3 81 AF EF BC  |................|
	00000020  81 0A 01 02 03 04 05 06 - 07 08 09 0A 0B 0C 0D 0E  |................|
	00000030  0F 10 11 12 13 14 15 16 - 17 18 19 1A 1B 1C 1D 1E  |................|
	00000040  1F 20 21 22 23 24 25 26 - 27 28 29 2A 2B 2C 2D 2E  |. !"#$%&'()*+,-.|
	00000050  2F 30 31 32 33 34 35 36 - 37 38 39 3A 3B 3C 3D 3E  |/0123456789:;<=>|
	00000060  3F 40 41 42 43 44 45 46 - 47 48 49 4A 4B 4C 4D 4E  |?@ABCDEFGHIJKLMN|
	00000070  4F 50 51 52 53 54 55 56 - 57 58 59 5A 5B 5C 5D 5E  |OPQRSTUVWXYZ[\]^|
	00000080  5F 60 61 62 63 64 65 66 - 67 68 69 6A 6B 6C 6D 6E  |_`abcdefghijklmn|
	00000090  6F 70 71 72 73 74 75 76 - 77 78 79 7A 7B 7C 7D 7E  |opqrstuvwxyz{|}~|
	000000A0  7F 80 81 82 83 84 85 86 - 87 88 89 8A 8B 8C 8D 8E  |................|
	000000B0  8F 90 91 92 93 94 95 96 - 97 98 99 9A 9B 9C 9D 9E  |................|
	000000C0  9F A0 A1 A2 A3 A4 A5 A6 - A7 A8 A9 AA AB AC AD AE  |................|
	000000D0  AF B0 B1 B2 B3 B4 B5 B6 - B7 B8 B9 BA BB BC BD BE  |................|
	000000E0  BF C0 C1 C2 C3 C4 C5 C6 - C7 C8 C9 CA CB CC CD CE  |................|
	000000F0  CF D0 D1 D2 D3 D4 D5 D6 - D7 D8 D9 DA DB DC DD DE  |................|
	00000100  DF E0 E1 E2 E3 E4 E5 E6 - E7 E8 E9 EA EB EC ED EE  |................|
	00000110  EF F0 F1 F2 F3 F4 F5 F6 - F7 F8 F9 FA FB FC FD FE  |................|
	00000120  FF 0A                   -                          |..|
---
name: integer-base-one-3b
description:
	some sample code for hexdumping Unicode
stdin:
	set -U
	{
		print 'Hello, World!\\\nこんにちは！'
		typeset -Uui16 i=0x100
		# change that to 0xFF once we can handle embedded
		# NUL characters in strings / here documents
		while (( i++ < 0x1FF )); do
			print -n "\u${i#16#1}"
		done
		print
		print \\xff		# invalid utf-8
		print \\xc2		# invalid 2-byte
		print \\xef\\xbf\\xc0	# invalid 3-byte
		print \\xc0\\x80	# non-minimalistic
		print \\xe0\\x80\\x80	# non-minimalistic
		print '�￾￿'	# end of range
	} | {
		typeset -Uui16 -Z11 pos=0
		typeset -Uui16 -Z5 hv
		typeset -i1 wc=0x0A
		dasc=
		nl=${wc#1#}
		integer n
		while IFS= read -r line; do
			line=$line$nl
			while [[ -n $line ]]; do
				(( hv = 1#${line::1} & 0xFF ))
				if (( (hv < 0xC2) || (hv >= 0xF0) )); then
					n=1
				elif (( hv < 0xE0 )); then
					n=2
				else
					n=3
				fi
				if (( n > 1 )); then
					(( (1#${line:1:1} & 0xC0) == 0x80 )) || n=1
					(( hv == 0xE0 )) && \
					    (( (1#${line:1:1} & 0xFF) < 0xA0 )) && n=1
				fi
				if (( n > 2 )); then
					(( hv = 1#${line:2:1} & 0xFF ))
					(( (hv & 0xC0) == 0x80 )) || n=1
					(( (((1#${line::1} & 0xFF) == 0xEF) && \
					    ((1#${line:1:1} & 0xFF) == 0xBF) && \
					    (hv > 0xBD)) )) && n=1
				fi
				wc=1#${line::n}
				if (( (wc < 32) || \
				    ((wc > 126) && (wc < 160)) )); then
					dch=.
				elif (( (wc & 0xFF80) == 0xEF80 )); then
					dch=�
				else
					dch=${wc#1#}
				fi
				if (( (pos & 15) >= (n == 3 ? 14 : 15) )); then
					dasc=$dasc$dch
					dch=
				fi
				while (( n-- )); do
					if (( (pos & 15) == 0 )); then
						(( pos )) && print "$dasc|"
						print -n "${pos#16#}  "
						dasc=' |'
					fi
					hv=1#${line::1}
					print -n "${hv#16#} "
					(( (pos++ & 15) == 7 )) && \
					    print -n -- '- '
					line=${line:1}
				done
				dasc=$dasc$dch
			done
		done
		if (( pos & 15 )); then
			while (( pos & 15 )); do
				print -n '   '
				(( (pos++ & 15) == 7 )) && print -n -- '- '
			done
			print "$dasc|"
		fi
	}
expected-stdout:
	00000000  48 65 6C 6C 6F 2C 20 57 - 6F 72 6C 64 21 5C 0A E3  |Hello, World!\.こ|
	00000010  81 93 E3 82 93 E3 81 AB - E3 81 A1 E3 81 AF EF BC  |んにちは！|
	00000020  81 0A 01 02 03 04 05 06 - 07 08 09 0A 0B 0C 0D 0E  |...............|
	00000030  0F 10 11 12 13 14 15 16 - 17 18 19 1A 1B 1C 1D 1E  |................|
	00000040  1F 20 21 22 23 24 25 26 - 27 28 29 2A 2B 2C 2D 2E  |. !"#$%&'()*+,-.|
	00000050  2F 30 31 32 33 34 35 36 - 37 38 39 3A 3B 3C 3D 3E  |/0123456789:;<=>|
	00000060  3F 40 41 42 43 44 45 46 - 47 48 49 4A 4B 4C 4D 4E  |?@ABCDEFGHIJKLMN|
	00000070  4F 50 51 52 53 54 55 56 - 57 58 59 5A 5B 5C 5D 5E  |OPQRSTUVWXYZ[\]^|
	00000080  5F 60 61 62 63 64 65 66 - 67 68 69 6A 6B 6C 6D 6E  |_`abcdefghijklmn|
	00000090  6F 70 71 72 73 74 75 76 - 77 78 79 7A 7B 7C 7D 7E  |opqrstuvwxyz{|}~|
	000000A0  7F C2 80 C2 81 C2 82 C2 - 83 C2 84 C2 85 C2 86 C2  |.........|
	000000B0  87 C2 88 C2 89 C2 8A C2 - 8B C2 8C C2 8D C2 8E C2  |........|
	000000C0  8F C2 90 C2 91 C2 92 C2 - 93 C2 94 C2 95 C2 96 C2  |........|
	000000D0  97 C2 98 C2 99 C2 9A C2 - 9B C2 9C C2 9D C2 9E C2  |........|
	000000E0  9F C2 A0 C2 A1 C2 A2 C2 - A3 C2 A4 C2 A5 C2 A6 C2  | ¡¢£¤¥¦§|
	000000F0  A7 C2 A8 C2 A9 C2 AA C2 - AB C2 AC C2 AD C2 AE C2  |¨©ª«¬­®¯|
	00000100  AF C2 B0 C2 B1 C2 B2 C2 - B3 C2 B4 C2 B5 C2 B6 C2  |°±²³´µ¶·|
	00000110  B7 C2 B8 C2 B9 C2 BA C2 - BB C2 BC C2 BD C2 BE C2  |¸¹º»¼½¾¿|
	00000120  BF C3 80 C3 81 C3 82 C3 - 83 C3 84 C3 85 C3 86 C3  |ÀÁÂÃÄÅÆÇ|
	00000130  87 C3 88 C3 89 C3 8A C3 - 8B C3 8C C3 8D C3 8E C3  |ÈÉÊËÌÍÎÏ|
	00000140  8F C3 90 C3 91 C3 92 C3 - 93 C3 94 C3 95 C3 96 C3  |ÐÑÒÓÔÕÖ×|
	00000150  97 C3 98 C3 99 C3 9A C3 - 9B C3 9C C3 9D C3 9E C3  |ØÙÚÛÜÝÞß|
	00000160  9F C3 A0 C3 A1 C3 A2 C3 - A3 C3 A4 C3 A5 C3 A6 C3  |àáâãäåæç|
	00000170  A7 C3 A8 C3 A9 C3 AA C3 - AB C3 AC C3 AD C3 AE C3  |èéêëìíîï|
	00000180  AF C3 B0 C3 B1 C3 B2 C3 - B3 C3 B4 C3 B5 C3 B6 C3  |ðñòóôõö÷|
	00000190  B7 C3 B8 C3 B9 C3 BA C3 - BB C3 BC C3 BD C3 BE C3  |øùúûüýþÿ|
	000001A0  BF 0A FF 0A C2 0A EF BF - C0 0A C0 80 0A E0 80 80  |.�.�.���.��.���|
	000001B0  0A EF BF BD EF BF BE EF - BF BF 0A                 |.�������.|
---
name: ulimit-1
description:
	Check if we can use a specific syntax idiom for ulimit
stdin:
	if ! x=$(ulimit -d); then
		print expected to fail on this OS
	else
		ulimit -dS $x && print okay
	fi
expected-stdout:
	okay
---
name: bashiop-1
description:
	Check if GNU bash-like I/O redirection works
	Part 1: this is also supported by GNU bash
stdin:
	exec 3>&1
	function threeout {
		echo ras
		echo dwa >&2
		echo tri >&3
	}
	threeout &>foo
	echo ===
	cat foo
expected-stdout:
	tri
	===
	ras
	dwa
---
name: bashiop-2a
description:
	Check if GNU bash-like I/O redirection works
	Part 2: this is *not* supported by GNU bash
stdin:
	exec 3>&1
	function threeout {
		echo ras
		echo dwa >&2
		echo tri >&3
	}
	threeout 3&>foo
	echo ===
	cat foo
expected-stdout:
	ras
	===
	dwa
	tri
---
name: bashiop-2b
description:
	Check if GNU bash-like I/O redirection works
	Part 2: this is *not* supported by GNU bash
stdin:
	exec 3>&1
	function threeout {
		echo ras
		echo dwa >&2
		echo tri >&3
	}
	threeout 3>foo &>&3
	echo ===
	cat foo
expected-stdout:
	===
	ras
	dwa
	tri
---
name: bashiop-2c
description:
	Check if GNU bash-like I/O redirection works
	Part 2: this is *not* supported by GNU bash
stdin:
	echo mir >foo
	set -o noclobber
	exec 3>&1
	function threeout {
		echo ras
		echo dwa >&2
		echo tri >&3
	}
	threeout &>>foo
	echo ===
	cat foo
expected-stdout:
	tri
	===
	mir
	ras
	dwa
---
name: bashiop-3a
description:
	Check if GNU bash-like I/O redirection fails correctly
	Part 1: this is also supported by GNU bash
stdin:
	echo mir >foo
	set -o noclobber
	exec 3>&1
	function threeout {
		echo ras
		echo dwa >&2
		echo tri >&3
	}
	threeout &>foo
	echo ===
	cat foo
expected-stdout:
	===
	mir
expected-stderr-pattern: /.*: cannot (create|overwrite) .*/
---
name: bashiop-3b
description:
	Check if GNU bash-like I/O redirection fails correctly
	Part 2: this is *not* supported by GNU bash
stdin:
	echo mir >foo
	set -o noclobber
	exec 3>&1
	function threeout {
		echo ras
		echo dwa >&2
		echo tri >&3
	}
	threeout &>|foo
	echo ===
	cat foo
expected-stdout:
	tri
	===
	ras
	dwa
---
name: bashiop-4
description:
	Check if GNU bash-like I/O redirection works
	Part 4: this is also supported by GNU bash,
	but failed in some mksh versions
stdin:
	exec 3>&1
	function threeout {
		echo ras
		echo dwa >&2
		echo tri >&3
	}
	function blubb {
		[[ -e bar ]] && threeout "$bf" &>foo
	}
	blubb
	echo -n >bar
	blubb
	echo ===
	cat foo
expected-stdout:
	tri
	===
	ras
	dwa
---
name: mkshiop-1
description:
	Check for support of more than 9 file descriptors
category: !smksh
stdin:
	read -u10 foo 10<<< bar
	print x$foo
expected-stdout:
	xbar
---
name: mkshiop-2
description:
	Check for support of more than 9 file descriptors
category: !smksh
stdin:
	exec 12>foo
	print -u12 bar
	print baz >&12
	cat foo
expected-stdout:
	bar
	baz
---
name: oksh-seterror
description:
	$OpenBSD: seterror.sh,v 1.1 2003/02/09 18:52:49 espie Exp $
	set -e is supposed to abort the script for errors that
	are not caught otherwise. pdksh fails this test.
stdin:
	set -e
	for i in 1 2 3
	do
		false && true
	done
	true
expected-fail: yes
---
name: oksh-shcrash
description:
	src/regress/bin/ksh/shcrash.sh,v 1.1
stdin:
	deplibs="-lz -lpng /usr/local/lib/libjpeg.la -ltiff -lm -lX11 -lXext /usr/local/lib/libiconv.la -L/usr/local/lib -L/usr/ports/devel/gettext/w-gettext-0.10.40/gettext-0.10.40/intl/.libs /usr/local/lib/libintl.la /usr/local/lib/libglib.la /usr/local/lib/libgmodule.la -lintl -lm -lX11 -lXext -L/usr/X11R6/lib -lglib -lgmodule -L/usr/local/lib /usr/local/lib/libgdk.la -lintl -lm -lX11 -lXext -L/usr/X11R6/lib -lglib -lgmodule -L/usr/local/lib /usr/local/lib/libgtk.la -ltiff -ljpeg -lz -lpng -lm -lX11 -lXext -lintl -lglib -lgmodule -lgdk -lgtk -L/usr/X11R6/lib -lglib -lgmodule -L/usr/local/lib /usr/local/lib/libgdk_pixbuf.la -lz -lpng /usr/local/lib/libiconv.la -L/usr/local/lib -L/usr/ports/devel/gettext/w-gettext-0.10.40/gettext-0.10.40/intl/.libs /usr/local/lib/libintl.la /usr/local/lib/libglib.la -lm -lm /usr/local/lib/libaudiofile.la -lm -lm -laudiofile -L/usr/local/lib /usr/local/lib/libesd.la -lm -lz -L/usr/local/lib /usr/local/lib/libgnomesupport.la -lm -lz -lm -lglib -L/usr/local/lib /usr/local/lib/libgnome.la -lX11 -lXext /usr/local/lib/libiconv.la -L/usr/local/lib -L/usr/ports/devel/gettext/w-gettext-0.10.40/gettext-0.10.40/intl/.libs /usr/local/lib/libintl.la /usr/local/lib/libgmodule.la -lintl -lm -lX11 -lXext -L/usr/X11R6/lib -lglib -lgmodule -L/usr/local/lib /usr/local/lib/libgdk.la -lintl -lm -lX11 -lXext -L/usr/X11R6/lib -lglib -lgmodule -L/usr/local/lib /usr/local/lib/libgtk.la -lICE -lSM -lz -lpng /usr/local/lib/libungif.la /usr/local/lib/libjpeg.la -ltiff -lm -lz -lpng /usr/local/lib/libungif.la -lz /usr/local/lib/libjpeg.la -ltiff -L/usr/local/lib -L/usr/X11R6/lib /usr/local/lib/libgdk_imlib.la -lm -L/usr/local/lib /usr/local/lib/libart_lgpl.la -lm -lz -lm -lX11 -lXext -lintl -lglib -lgmodule -lgdk -lgtk -lICE -lSM -lm -lX11 -lXext -lintl -lglib -lgmodule -lgdk -lgtk -L/usr/X11R6/lib -lm -lz -lpng -lungif -lz -ljpeg -ltiff -ljpeg -lgdk_imlib -lglib -lm -laudiofile -lm -laudiofile -lesd -L/usr/local/lib /usr/local/lib/libgnomeui.la -lz -lz /usr/local/lib/libxml.la -lz -lz -lz /usr/local/lib/libxml.la -lm -lX11 -lXext /usr/local/lib/libiconv.la -L/usr/ports/devel/gettext/w-gettext-0.10.40/gettext-0.10.40/intl/.libs /usr/local/lib/libintl.la /usr/local/lib/libglib.la /usr/local/lib/libgmodule.la -lintl -lglib -lgmodule /usr/local/lib/libgdk.la /usr/local/lib/libgtk.la -L/usr/X11R6/lib -L/usr/local/lib /usr/local/lib/libglade.la -lz -lz -lz /usr/local/lib/libxml.la /usr/local/lib/libglib.la -lm -lm /usr/local/lib/libaudiofile.la -lm -lm -laudiofile /usr/local/lib/libesd.la -lm -lz /usr/local/lib/libgnomesupport.la -lm -lz -lm -lglib /usr/local/lib/libgnome.la -lX11 -lXext /usr/local/lib/libiconv.la -L/usr/ports/devel/gettext/w-gettext-0.10.40/gettext-0.10.40/intl/.libs /usr/local/lib/libintl.la /usr/local/lib/libgmodule.la -lintl -lm -lX11 -lXext -lglib -lgmodule /usr/local/lib/libgdk.la -lintl -lm -lX11 -lXext -lglib -lgmodule /usr/local/lib/libgtk.la -lICE -lSM -lz -lpng /usr/local/lib/libungif.la /usr/local/lib/libjpeg.la -ltiff -lm -lz -lz /usr/local/lib/libgdk_imlib.la /usr/local/lib/libart_lgpl.la -lm -lz -lm -lX11 -lXext -lintl -lglib -lgmodule -lgdk -lgtk -lm -lX11 -lXext -lintl -lglib -lgmodule -lgdk -lgtk -lm -lz -lungif -lz -ljpeg -ljpeg -lgdk_imlib -lglib -lm -laudiofile -lm -laudiofile -lesd /usr/local/lib/libgnomeui.la -L/usr/X11R6/lib -L/usr/local/lib /usr/local/lib/libglade-gnome.la /usr/local/lib/libglib.la -lm -lm /usr/local/lib/libaudiofile.la -lm -lm -laudiofile -L/usr/local/lib /usr/local/lib/libesd.la -lm -lz -L/usr/local/lib /usr/local/lib/libgnomesupport.la -lm -lz -lm -lglib -L/usr/local/lib /usr/local/lib/libgnome.la -lX11 -lXext /usr/local/lib/libiconv.la -L/usr/local/lib -L/usr/ports/devel/gettext/w-gettext-0.10.40/gettext-0.10.40/intl/.libs /usr/local/lib/libintl.la /usr/local/lib/libgmodule.la -lintl -lm -lX11 -lXext -L/usr/X11R6/lib -lglib -lgmodule -L/usr/local/lib /usr/local/lib/libgdk.la -lintl -lm -lX11 -lXext -L/usr/X11R6/lib -lglib -lgmodule -L/usr/local/lib /usr/local/lib/libgtk.la -lICE -lSM -lz -lpng /usr/local/lib/libungif.la /usr/local/lib/libjpeg.la -ltiff -lm -lz -lpng /usr/local/lib/libungif.la -lz /usr/local/lib/libjpeg.la -ltiff -L/usr/local/lib -L/usr/X11R6/lib /usr/local/lib/libgdk_imlib.la -lm -L/usr/local/lib /usr/local/lib/libart_lgpl.la -lm -lz -lm -lX11 -lXext -lintl -lglib -lgmodule -lgdk -lgtk -lICE -lSM -lm -lX11 -lXext -lintl -lglib -lgmodule -lgdk -lgtk -L/usr/X11R6/lib -lm -lz -lpng -lungif -lz -ljpeg -ltiff -ljpeg -lgdk_imlib -lglib -lm -laudiofile -lm -laudiofile -lesd -L/usr/local/lib /usr/local/lib/libgnomeui.la -L/usr/X11R6/lib -L/usr/local/lib"
	specialdeplibs="-lgnomeui -lart_lgpl -lgdk_imlib -ltiff -ljpeg -lungif -lpng -lz -lSM -lICE -lgtk -lgdk -lgmodule -lintl -lXext -lX11 -lgnome -lgnomesupport -lesd -laudiofile -lm -lglib"
	for deplib in $deplibs; do
		case $deplib in
		-L*)
			new_libs="$deplib $new_libs"
			;;
		*)
			case " $specialdeplibs " in
			*" $deplib "*)
				new_libs="$deplib $new_libs";;
			esac
			;;
		esac
	done
---
name: oksh-varfunction
description:
	$OpenBSD: varfunction.sh,v 1.1 2003/12/15 05:28:40 otto Exp $
	Calling
		FOO=bar f
	where f is a ksh style function, should not set FOO in the current
	env. If f is a bourne style function, FOO should be set. Furthermore,
	the function should receive a correct value of FOO. Additionally,
	setting FOO in the function itself should not change the value in
	global environment.
	Inspired by PR 2450.
stdin:
	function k {
		if [ x$FOO != xbar ]; then
			echo 1
			return 1
		fi
		x=$(env | grep FOO)
		if [ "x$x" != "xFOO=bar" ]; then
			echo 2
			return 1;
		fi
		FOO=foo
		return 0
	}
	b () {
		if [ x$FOO != xbar ]; then
			echo 3
			return 1
		fi
		x=$(env | grep FOO)
		if [ "x$x" != "xFOO=bar" ]; then
			echo 4
			return 1;
		fi
		FOO=foo
		return 0
	}
	FOO=bar k
	if [ $? != 0 ]; then
		exit 1
	fi
	if [ x$FOO != x ]; then
		exit 1
	fi
	FOO=bar b
	if [ $? != 0 ]; then
		exit 1
	fi
	if [ x$FOO != xbar ]; then
		exit 1
	fi
	FOO=barbar
	FOO=bar k
	if [ $? != 0 ]; then
		exit 1
	fi
	if [ x$FOO != xbarbar ]; then
		exit 1
	fi
	FOO=bar b
	if [ $? != 0 ]; then
		exit 1
	fi
	if [ x$FOO != xbar ]; then
		exit 1
	fi
---
name: fd-cloexec-1
description:
	Verify that file descriptors > 2 are private for Korn shells
file-setup: file 644 "test.sh"
	print -u3 Fowl
stdin:
	exec 3>&1
	"$__progname" test.sh
expected-exit: e != 0
expected-stderr:
	test.sh[1]: print: -u: 3: bad file descriptor
---
name: fd-cloexec-2
description:
	Verify that file descriptors > 2 are not private for POSIX shells
	See Debian Bug #154540, Closes: #499139
file-setup: file 644 "test.sh"
	print -u3 Fowl
stdin:
	set -o posix
	exec 3>&1
	"$__progname" test.sh
expected-stdout:
	Fowl
---
