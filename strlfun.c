/* $MirOS: src/bin/mksh/strlfun.c,v 1.6 2006/11/08 23:23:41 tg Exp $ */
/* _MirOS: src/lib/libc/string/strlfun.c,v 1.10 2006/11/08 23:18:04 tg Exp $ */

/*-
 * Copyright (c) 2006
 *	Thorsten Glaser <tg@mirbsd.de>
 *
 * Licensee is hereby permitted to deal in this work without restric-
 * tion, including unlimited rights to use, publicly perform, modify,
 * merge, distribute, sell, give away or sublicence, provided all co-
 * pyright notices above, these terms and the disclaimer are retained
 * in all redistributions or reproduced in accompanying documentation
 * or other materials provided with binary redistributions.
 *
 * Licensor offers the work "AS IS" and WITHOUT WARRANTY of any kind,
 * express, or implied, to the maximum extent permitted by applicable
 * law, without malicious intent or gross negligence; in no event may
 * licensor, an author or contributor be held liable for any indirect
 * or other damage, or direct damage except proven a consequence of a
 * direct error of said person and intended use of this work, loss or
 * other issues arising in any way out of its use, even if advised of
 * the possibility of such damage or existence of a defect.
 *-
 * The strlcat() code below has been written by Thorsten Glaser. Bodo
 * Eggert suggested optimising the strlcpy() code, originally written
 * by Todd C. Miller (see below), which was carried out by Th. Glaser
 * as well as writing wcslcat() and wcslcpy() equivalents.
 */

#include <sys/types.h>
#if defined(_KERNEL) || defined(_STANDALONE)
#include <lib/libkern/libkern.h>
#undef HAVE_STRLCPY
#undef HAVE_STRLCAT
#else
#if defined(HAVE_CONFIG_H) && (HAVE_CONFIG_H != 0)
/* usually when packaged with third-party software */
#ifdef CONFIG_H_FILENAME
#include CONFIG_H_FILENAME
#else
#include "config.h"
#endif
#endif
extern size_t strlen(const char *);
#endif

#ifndef __RCSID
#undef __IDSTRING
#undef __IDSTRING_CONCAT
#undef __IDSTRING_EXPAND
#if defined(__ELF__) && defined(__GNUC__)
#define __IDSTRING(prefix, string)				\
	__asm__(".section .comment"				\
	"\n	.ascii	\"@(\"\"#)" #prefix ": \""		\
	"\n	.asciz	\"" string "\""				\
	"\n	.previous")
#else
#define __IDSTRING_CONCAT(l,p)		__LINTED__ ## l ## _ ## p
#define __IDSTRING_EXPAND(l,p)		__IDSTRING_CONCAT(l,p)
#define __IDSTRING(prefix, string)				\
	static const char __IDSTRING_EXPAND(__LINE__,prefix) []	\
	    __attribute__((used)) = "@(""#)" #prefix ": " string
#endif
#define __RCSID(x)		__IDSTRING(rcsid,x)
#endif

#ifndef __predict_true
#define __predict_true(exp)	((exp) != 0)
#endif
#ifndef __predict_false
#define __predict_false(exp)	((exp) != 0)
#endif

__RCSID("$MirOS: src/bin/mksh/strlfun.c,v 1.6 2006/11/08 23:23:41 tg Exp $");

size_t strlcat(char *, const char *, size_t);
size_t strlcpy(char *, const char *, size_t);

#if !defined(HAVE_STRLCAT) || (HAVE_STRLCAT == 0)
/*
 * Appends src to string dst of size siz (unlike strncat, siz is the
 * full size of dst, not space left).  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz <= strlen(dst)).
 * Returns strlen(src) + MIN(siz, strlen(initial dst)).
 * If retval >= siz, truncation occurred.
 */
size_t
strlcat(char *dst, const char *src, size_t dlen)
{
	size_t n = 0, slen;

	slen = strlen(src);
	while (__predict_true(n + 1 < dlen && dst[n] != '\0'))
		++n;
	if (__predict_false(dlen == 0 || dst[n] != '\0'))
		return (dlen + slen);
	while (__predict_true((slen > 0) && (n < (dlen - 1)))) {
		dst[n++] = *src++;
		--slen;
	}
	dst[n] = '\0';
	return (n + slen);
}
#endif /* !HAVE_STRLCAT */

/* $OpenBSD: strlcpy.c,v 1.10 2005/08/08 08:05:37 espie Exp $ */

/*-
 * Copyright (c) 1998 Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 */

#if !defined(HAVE_STRLCPY) || (HAVE_STRLCPY == 0)
/*
 * Copy src to string dst of size siz.  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz == 0).
 * Returns strlen(src); if retval >= siz, truncation occurred.
 */
size_t
strlcpy(char *dst, const char *src, size_t siz)
{
	const char *s = src;

	if (__predict_false(siz == 0))
		goto traverse_src;

	/* copy as many chars as will fit */
	while (--siz && (*dst++ = *s++))
		;

	/* not enough room in dst */
	if (__predict_false(siz == 0)) {
		/* safe to NUL-terminate dst since we copied <= siz-1 chars */
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
