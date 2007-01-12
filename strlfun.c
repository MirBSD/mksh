/* $MirOS: src/bin/mksh/strlfun.c,v 1.9 2007/01/12 01:49:29 tg Exp $ */
/* $miros: src/lib/libc/string/strlfun.c,v 1.14 2007/01/07 02:11:40 tg Exp $ */

/*-
 * Copyright (c) 2006
 *	Thorsten Glaser <tg@mirbsd.de>
 *
 * This work is provided "AS IS" and WITHOUT WARRANTY of any kind, to
 * the utmost extent permitted by applicable law, neither express nor
 * implied; without malicious intent or gross negligence. In no event
 * may a licensor, author or contributor be held liable for indirect,
 * direct, other damage, loss, or other issues arising in any way out
 * of dealing in the work, even if advised of the possibility of such
 * damage or existence of a defect, except proven that it results out
 * of said person's immediate fault when using the work as intended.
 *-
 * The strlcat() code below has been written by Thorsten Glaser. Bodo
 * Eggert suggested optimising the strlcpy() code, originally written
 * by Todd C. Miller (see below), which was carried out by Th. Glaser
 * as well as merging this code with strxfrm() for ISO-10646-only sy-
 * stems and writing wcslcat(), wcslcpy() and wcsxfrm() equivalents.
 */

#ifdef STRXFRM
#undef HAVE_STRLCPY
#undef HAVE_STRLCAT
#define HAVE_STRLCPY	0
#define HAVE_STRLCAT	1
#define strlcpy		strxfrm
#endif

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
#endif

#ifndef __predict_true
#define __predict_true(exp)	((exp) != 0)
#endif
#ifndef __predict_false
#define __predict_false(exp)	((exp) != 0)
#endif

#if !defined(_KERNEL) && !defined(_STANDALONE)
__RCSID("$MirOS: src/bin/mksh/strlfun.c,v 1.9 2007/01/12 01:49:29 tg Exp $");
#endif

size_t strlcpy(char *, const char *, size_t);

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
