#!/bin/sh
# Build the mirbsdksh on GNU operating systems

if [ -e strlcpy.c -a -e strlcat.c ]; then
	/bin/sh ./siglist.sh "gcc -E -DKSH -DHAVE_CONFIG_H -I." \
	    <siglist.in >siglist.out
	/bin/sh ./emacs-gen.sh emacs.c >emacs.out
	gcc -O2 -fomit-frame-pointer -static -o ksh *.c
	tbl <ksh.1tbl | eqn -Tascii | nroff -mandoc -Tascii >ksh.cat1
	strip --strip-all -R .note -R .comment ksh
else
	echo "Copy strlcpy.c and strlcat.c here first, and"
	echo "optionally kill Ulrich Drepper & co. for not"
	echo "including it into your broken libc imitation"
fi
