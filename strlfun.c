/* $MirBSD: strlfun.c,v 1.1.2.1 2004/05/24 17:43:42 tg Rel $ */
/* _MirBSD: src/lib/libc/string/strlfun.c,v 1.2 2004/04/27 18:52:23 tg Exp $ */
/* $OpenBSD: strlcpy.c,v 1.8 2003/06/17 21:56:24 millert Exp $ */
/* $OpenBSD: strlcat.c,v 1.11 2003/06/17 21:56:24 millert Exp $ */

/* Copyright (c) 2004
 *	Thorsten "mirabile" Glaser <x86@ePost.de>
 *
 * Subject to these terms, everybody who obtained a copy of this work
 * is hereby permitted to deal in the work without restriction inclu-
 * ding without limitation the rights to use, distribute, sell, modi-
 * fy, publically perform, give away, merge or sublicence it provided
 * this notice is kept and the authors and contributors are given due
 * credit in derivates or accompanying documents.
 *
 * This work is provided by its developers (authors and contributors)
 * "as is" and without any warranties whatsoever, express or implied,
 * to the maximum extent permitted by applicable law; in no event may
 * developers be held liable for damage caused, directly or indirect-
 * ly, but not by a developer's malice intent, even if advised of the
 * possibility of such damage.
 */
/*
 * Some optimizations idea from Bodo Eggert
 */
/*
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

#ifdef	HAVE_CONFIG_H
/* packaged with third-party software */
#include "config.h"
#define	LIBC_SCCS
#else	/* ! def(HAVE_CONFIG_H) */
/* integrated into C library */
#undef	HAVE_STRLCPY
#undef	HAVE_STRLCAT
#endif	/* ! def(HAVE_CONFIG_H) */

#if defined(LIBC_SCCS) && !defined(lint)
static const char rcsid[] = "$MirBSD: strlfun.c,v 1.1.2.1 2004/05/24 17:43:42 tg Rel $";
#endif	/* LIBC_SCCS and not lint */

#include <sys/types.h>
#include <string.h>


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
#endif	/* ndef HAVE_STRLCPY */

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
	size_t n = siz, dl;
	const size_t sl = strlen(src);

	while (n-- && (*d++ != '\0'))
		;
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
#endif	/* ndef HAVE_STRLCAT */
