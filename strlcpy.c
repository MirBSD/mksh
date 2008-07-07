/*-
 * Copyright (c) 2006, 2008
 *	Thorsten Glaser <tg@mirbsd.org>
 * Copyright (c) 1998 Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * This work is provided "AS IS" and WITHOUT WARRANTY of any kind, to
 * the utmost extent permitted by applicable law, neither express nor
 * implied; without malicious intent or gross negligence. In no event
 * may a licensor, author or contributor be held liable for indirect,
 * direct, other damage, loss, or other issues arising in any way out
 * of dealing in the work, even if advised of the possibility of such
 * damage or existence of a defect, except proven that it results out
 * of said person's immediate fault when using the work as intended.
 */

#include "sh.h"

__RCSID("$MirOS: src/bin/mksh/strlcpy.c,v 1.3 2008/07/07 12:59:54 tg Stab $");
__RCSID("$miros: src/lib/libc/string/strlfun.c,v 1.16 2008/07/07 12:59:51 tg Stab $");

#ifndef __predict_true
#define __predict_true(exp)	((exp) != 0)
#endif
#ifndef __predict_false
#define __predict_false(exp)	((exp) != 0)
#endif

/* (multibyte) string functions */
#undef NUL
#undef char_t
#define NUL		'\0'
#define char_t		char

/*
 * Copy src to string dst of size siz.  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz == 0).
 * Returns strlen(src); if retval >= siz, truncation occurred.
 */
size_t
strlcpy(char_t *dst, const char_t *src, size_t siz)
{
	const char_t *s = src;

	if (__predict_false(siz == 0))
		goto traverse_src;

	/* copy as many chars as will fit */
	while (--siz && (*dst++ = *s++))
		;

	/* not enough room in dst */
	if (__predict_false(siz == 0)) {
		/* safe to NUL-terminate dst since we copied <= siz-1 chars */
		*dst = NUL;
 traverse_src:
		/* traverse rest of src */
		while (*s++)
			;
	}

	/* count does not include NUL */
	return (s - src - 1);
}
