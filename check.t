# $MirOS: src/bin/mksh/check.t,v 1.56.2.1 2006/08/18 19:02:21 tg Exp $
# $OpenBSD: bksl-nl.t,v 1.2 2001/01/28 23:04:56 niklas Exp $
# $OpenBSD: history.t,v 1.5 2001/01/28 23:04:56 niklas Exp $
# $OpenBSD: read.t,v 1.3 2003/03/10 03:48:16 david Exp $
#-
# You may also want to test IFS with the script at
# http://www.research.att.com/~gsf/public/ifs.sh

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
name: blsk-nl-ign-4
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
name: blsk-nl-ign-5
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
name: blsk-nl-9
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
name: blsk-nl-10
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
name: blsk-nl-ksh-1
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
name: blsk-nl-ksh-2
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
	Eglobing in trim expressions...
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
	Check eglobing works in trims...
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
# breaks on Mac OSX (probably UTF-8 issue)
category: !os:darwin
file-setup: file 644 "aÂc"
stdin:
	echo a[Á-Ú]*
expected-stdout:
	aÂc
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
	Check that heredoc temp files aren't removed too soon or too late.
	Heredoc in function, backgrounded call to function.
	This check can fail on slow machines (<100 MHz), that's normal.
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
	fc -e - echo
	fc -l 2 4
expected-stdout:
	abc def
	ghi jkl
	ghi jkl
	2	echo ghi jkl
	3	echo ghi jkl
	4	fc -l 2 4
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
	the character count to stdout. Found on MS Interix/SFU 3.5
	and Mac OSX 10.4 "Tiger".
category: os:interix,os:darwin,os:freebsd
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
category: os:interix,os:darwin,os:freebsd
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
category: os:interix,os:darwin,os:freebsd
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
# we don't have persistent history on Solaris (no flock)
category: !os:solaris,!os:interix,!os:darwin,!os:freebsd
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
category: !os:solaris,!os:interix,!os:darwin,!os:freebsd
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
category: !os:solaris,!os:interix,!os:darwin,!os:freebsd
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
	PS1= # minimize prompt hassles
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
	j=~3
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
	(: ; cat t2) | sleep 1
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
	Parameter assignments with [] recognized correctly
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
	TMPDIR=$PWD/foo "$0" <<- 'EOF'
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
	TMPDIR=$PWD/foo "$0" <<- 'EOF'
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
	Check that globing works in pipelined commands
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
	uninitialized array elements.
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
	echo `echo [£`
expected-stdout:
	[£
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
	set +o | fgrep -q posix && echo posix || echo noposix
	set +o | fgrep -q braceexpand && echo brex || echo nobrex
	echo {a,b,c}
	set +o braceexpand
	echo {a,b,c}
	set -o braceexpand
	echo {a,b,c}
	set -o posix
	echo {a,b,c}
	set +o | fgrep -q posix && echo posix || echo noposix
	set +o | fgrep -q braceexpand && echo brex || echo nobrex
	set -o braceexpand
	echo {a,b,c}
	set +o | fgrep -q posix && echo posix || echo noposix
	set +o | fgrep -q braceexpand && echo brex || echo nobrex
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
name: pipeline-subshell-1
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
name: version-1
description:
	Check version of shell.
category: pdksh
stdin:
	echo $KSH_VERSION
expected-stdout:
	@(#)MIRBSD KSH R28 2006/08/18
---
