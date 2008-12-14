/*	$OpenBSD: alloc.c,v 1.8 2008/07/21 17:30:08 millert Exp $	*/

/*-
 * Copyright (c) 2002 Marc Espie.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE OPENBSD PROJECT AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE OPENBSD
 * PROJECT OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *-
 * area-based allocation built on malloc/free
 */

#include "sh.h"

__RCSID("$MirOS: src/bin/mksh/alloc.c,v 1.6.2.2 2008/12/14 00:07:32 tg Exp $");

struct link {
	struct link *prev;
	struct link *next;
};

Area *
ainit(Area *ap)
{
	ap->freelist = NULL;
	return (ap);
}

void
afreeall(Area *ap)
{
	struct link *l, *l2;

	for (l = ap->freelist; l != NULL; l = l2) {
		l2 = l->next;
		free(l);
	}
	ap->freelist = NULL;
}

#define L2P(l)	( (void *)(((char *)(l)) + sizeof (struct link)) )
#define P2L(p)	( (struct link *)(((ptrdiff_t)(p)) - sizeof (struct link)) )

void *
alloc(size_t size, Area *ap)
{
	struct link *l;

	if ((l = malloc(sizeof (struct link) + size)) == NULL)
		internal_errorf("unable to allocate memory");
	l->next = ap->freelist;
	l->prev = NULL;
	if (ap->freelist)
		ap->freelist->prev = l;
	ap->freelist = l;

	return (L2P(l));
}

void *
aresize(void *ptr, size_t size, Area *ap)
{
	struct link *l, *l2, *lprev, *lnext;

	if (ptr == NULL)
		return (alloc(size, ap));

	l = P2L(ptr);
	lprev = l->prev;
	lnext = l->next;

	if ((l2 = realloc(l, sizeof (struct link) + size)) == NULL)
		internal_errorf("unable to allocate memory");
	if (lprev)
		lprev->next = l2;
	else
		ap->freelist = l2;
	if (lnext)
		lnext->prev = l2;

	return (L2P(l2));
}

void
afree(void *ptr, Area *ap)
{
	struct link *l;
#ifdef MKSH_AFREE_DEBUG
	struct link *lp;
#endif

	if (!ptr)
		return;

	l = P2L(ptr);

#ifdef MKSH_AFREE_DEBUG
	for (lp = ap->freelist; lp != NULL; lp = lp->next)
		if (l == lp)
			break;
	if (lp == NULL) {
		internal_warningf("afree: pointer %p not allocated", ptr);
		exit(255);
	}
#endif

	if (l->prev)
		l->prev->next = l->next;
	else
		ap->freelist = l->next;
	if (l->next)
		l->next->prev = l->prev;

	free(l);
}
