/* $MirOS: src/bin/mksh/strlfun.c,v 1.3 2006/08/01 13:43:28 tg Exp $ */
/* _MirOS: src/lib/libc/string/strlfun.c,v 1.7 2006/08/01 13:41:49 tg Exp $ */
/* $OpenBSD: strlcpy.c,v 1.10 2005/08/08 08:05:37 espie Exp $ */
/* $OpenBSD: strlcat.c,v 1.13 2005/08/08 08:05:37 espie Exp $ */

/*-
 * Copyright (c) 2004, 2005, 2006 Thorsten Glaser <tg@mirbsd.de>
 * Thanks to Bodo Eggert for optimisation hints
 * Copyright (c) 1998 Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#if defined(_KERNEL) || defined(_STANDALONE)
# include <lib/libkern/libkern.h>
# undef HAVE_STRLCPY
# undef HAVE_STRLCAT
#else
# ifdef HAVE_CONFIG_H	/* usually when packaged with third-party software */
#  include "config.h"
# endif
# include <sys/types.h>

extern size_t strlen(const char *);

#ifndef __RCSID
#define __RCSID(x)	static const char __rcsid[] = (x)
#endif

#ifndef __predict_false
#define __predict_false(exp)	((exp) != 0)
#endif

__RCSID("$MirOS: src/bin/mksh/strlfun.c,v 1.3 2006/08/01 13:43:28 tg Exp $");
#endif

size_t strlcat(char *, const char *, size_t);
size_t strlcpy(char *, const char *, size_t);

#ifndef	HAVE_STRLCPY
/*
 * Copy src to string dst of size siz.  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz == 0).
 * Returns strlen(src); if retval >= siz, truncation occurred.
 */
size_t
strlcpy(char *dst, const char *src, size_t siz)
{
	const char *s = src;

	if (__predict_false(!siz))
		goto traverse_src;

	/* copy as many chars as will fit */
	for (; --siz && (*dst++ = *s++); )
		;

	/* not enough room in dst */
	if (__predict_false(!siz)) {
		/* safe to NUL-terminate dst since copied <= siz-1 chars */
		*dst = '\0';
 traverse_src:
		/* traverse rest of src */
		while (*s++)
			;
	}

	/* count doesn't include NUL */
	return (s - src - 1);
}
#endif /* !HAVE_STRLCPY */

#ifndef	HAVE_STRLCAT
/*
 * Appends src to string dst of size siz (unlike strncat, siz is the
 * full size of dst, not space left).  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz <= strlen(dst)).
 * Returns strlen(src) + MIN(siz, strlen(initial dst)).
 * If retval >= siz, truncation occurred.
 */
size_t
strlcat(char *dst, const char *src, size_t siz)
{
	char *d = dst;
	size_t dl, n = siz;
	const size_t sl = strlen(src);

	while (n-- && (*d++ != '\0'))
		;
	if (!++n && (*d != '\0'))
		return (strlen(src));

	dl = --d - dst;		/* original strlen(dst), max. siz-1 */
	n = siz - dl;
	dl += sl;

	if (__predict_false(!n--))
		return (dl);

	if (__predict_false(n > sl))
		n = sl;		/* number of chars to copy */
	for (; n-- && (*d++ = *src++); )
		;
	*d = '\0';		/* NUL-terminate dst */
	return (dl);
}
#endif /* !HAVE_STRLCAT */
