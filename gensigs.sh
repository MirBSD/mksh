#!/bin/sh
# $MirOS: src/bin/mksh/gensigs.sh,v 1.1 2005/05/23 12:01:09 tg Exp $

echo '#include <signal.h>' | $CC -E -dD -D_ANSI_SOURCE - \
    | grep '[	 ]SIG[A-Z0-9]*[	 ]' \
    | sed 's/^\(.*[	 ]SIG\)\([A-Z0-9]*\)\([	 ].*\)$/\2/' \
    | while read name; do
	( echo '#include <signal.h>'; echo "__mksh_test: SIG$name" ) \
	    | $CC -E - | fgrep __mksh_test: \
	    | sed 's/^__mksh_test: \([0-9]*\).*$/    { \1, "'$name'" },/'
done | fgrep -v '{ ,' >signames.inc
