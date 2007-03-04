#ifndef MKSH_SIGNAMES_CHECK
__RCSID("$MirOS: src/bin/mksh/signames.c,v 1.2 2007/03/04 03:04:28 tg Exp $");
#endif

static const struct mksh_sigpair {
	int nr;
	const char *const name;
} mksh_sigpairs[] = {
#ifdef __Plan9__
...
#elif defined(__minix) && !defined(__GNUC__)
...
#elif defined(MKSH_SIGNAMES_CHECK)
#error no, must be OS supplied
#elif !defined(__IN_MKDEP)
#include "signames.inc"
#endif
	{ 0, NULL }
};
