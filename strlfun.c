/**	$MirOS: src/bin/mksh/strlfun.c,v 1.1.7.2 2005/05/28 21:02:03 tg Exp $ */
/**	_MirOS: src/lib/libc/string/strlfun.c,v 1.4 2005/05/28 20:59:09 tg Exp $ */
/*	$OpenBSD: strlcpy.c,v 1.8 2003/06/17 21:56:24 millert Exp $ */
/*	$OpenBSD: strlcat.c,v 1.11 2003/06/17 21:56:24 millert Exp $ */

/*-
 * Copyright (c) 2004, 2005 Thorsten "mirabile" Glaser <tg@66h.42h.de>
 * Some hints for optimisation from Bodo Eggert (via d.a.s.r)
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

#if !defined(_KERNEL) && !defined(_STANDALONE)
#ifdef HAVE_CONFIG_H
/* usually when packaged with third-party software */
#include "config.h"
#define LIBC_SCCS
#endif
#include <sys/types.h>

extern size_t strlen(const char *);

#ifndef __RCSID
#define __RCSID(x)	static const char __rcsid[] = (x)
#endif

__RCSID("$MirOS: src/bin/mksh/strlfun.c,v 1.1.7.2 2005/05/28 21:02:03 tg Exp $");
#else
#include <lib/libkern/libkern.h>
#undef HAVE_CONFIG_H
#endif

#ifndef HAVE_CONFIG_H
#undef HAVE_STRLCPY
#undef HAVE_STRLCAT
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

	if (!siz) goto traverse_src;

	/* Copy as many bytes as will fit */
	for (; --siz && (*dst++ = *s++); /* nothing */)
		;

	/* Not enough room in dst, add NUL and traverse rest of src */
	if (!siz) {
		/* Save, since we've copied at max. (siz-1) characters */
		*dst = '\0';	/* NUL-terminate dst */
traverse_src:
		while (*s++)
			;
	}

	return (s - src - 1);	/* count does not include NUL */
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
		return strlen(src);

	dl = --d - dst;		/* original strlen(dst), max. siz-1 */
	n = siz - dl;
	dl += sl;

	if (!n--)
		return dl;

	if (n > sl)
		n = sl;		/* number of octets to copy */
	for (; n-- && (*d++ = *src++); /* nothing */)
		;
	*d = '\0';		/* NUL-terminate dst */
	return dl;
}
#endif /* !HAVE_STRLCAT */
