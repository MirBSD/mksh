/**	$MirBSD: src/bin/ksh/strlfun.c,v 1.1 2004/12/10 18:12:29 tg Exp $ */
/**	_MirBSD: src/lib/libc/string/strlfun.c,v 1.7 2004/12/05 16:07:53 tg Exp $ */
/*	$OpenBSD: strlcpy.c,v 1.8 2003/06/17 21:56:24 millert Exp $ */
/*	$OpenBSD: strlcat.c,v 1.11 2003/06/17 21:56:24 millert Exp $ */

/*-
 * Copyright (c) 2004
 *	Thorsten "mirabile" Glaser <tg@66h.42h.de>
 *
 * Licensee is hereby permitted to deal in this work without restric-
 * tion, including unlimited rights to use, publicly perform, modify,
 * merge, distribute, sell, give away or sublicence, provided all co-
 * pyright notices above, these terms and the disclaimer are retained
 * in all redistributions or reproduced in accompanying documentation
 * or other materials provided with binary redistributions.
 *
 * Licensor hereby provides this work "AS IS" and WITHOUT WARRANTY of
 * any kind, expressed or implied, to the maximum extent permitted by
 * applicable law, but with the warranty of being written without ma-
 * licious intent or gross negligence; in no event shall licensor, an
 * author or contributor be held liable for any damage, direct, indi-
 * rect or other, however caused, arising in any way out of the usage
 * of this work, even if advised of the possibility of such damage.
 *-
 * Implementation for most of this code by myself.
 * Some optimisation ideas from Bodo Eggert (via d.a.s.r).
 * The rest of the code is covered by the terms below:
 *-
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


#include <sys/types.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
/* packaged with third-party software */
#include "config.h"
#define LIBC_SCCS
#else /* ! def HAVE_CONFIG_H */
/* integrated into MirOS C library */
#undef HAVE_STRLCPY
#undef HAVE_STRLCAT
#endif /* ! def HAVE_CONFIG_H */

#ifndef __RCSID
#define __RCSID(x)	static const char __rcsid[] = (x)
#endif

__RCSID("$MirBSD: src/bin/ksh/strlfun.c,v 1.1 2004/12/10 18:12:29 tg Exp $");


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
#endif	/* ndef HAVE_STRLCAT */
