#!/bin/sh
# $MirOS: src/bin/mksh/gensigs.sh,v 1.3 2006/06/23 15:05:39 tg Exp $

echo '#include <signal.h>' | $CC $CPPFLAGS -E -dD -D_ANSI_SOURCE - \
    | grep '[	 ]SIG[A-Z0-9]*[	 ]' \
    | sed 's/^\(.*[	 ]SIG\)\([A-Z0-9]*\)\([	 ].*\)$/\2/' \
    | while read name; do
	( echo '#include <signal.h>'; echo "__mksh_test: SIG$name" ) \
	    | $CC $CPPFLAGS -E - | fgrep __mksh_test: | sed \
	    's/^__mksh_test: \([0-9]*\).*$/		{ \1, "'$name'" },/'
done | fgrep -v '{ ,' >signames.inc
