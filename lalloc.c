/*-
 * Copyright (c) 2009, 2010, 2011, 2013, 2014, 2016, 2021, 2022
 *	mirabilos <m$(date +%Y)@mirbsd.de>
 *
 * Provided that these terms and disclaimer and all copyright notices
 * are retained or reproduced in an accompanying document, permission
 * is granted to deal in this work without restriction, including un-
 * limited rights to use, publicly perform, distribute, sell, modify,
 * merge, give away, or sublicence.
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

#ifdef MKSH_ALLOC_CATCH_UNDERRUNS
#include <err.h>
#endif
#include "sh.h"

__RCSID("$MirOS: src/bin/mksh/lalloc.c,v 1.36 2025/04/25 23:14:57 tg Exp $");

/* build with CPPFLAGS+= -DUSE_REALLOC_MALLOC=0 on ancient systems */
#if defined(USE_REALLOC_MALLOC) && (USE_REALLOC_MALLOC == 0)
#define remalloc(p,n)	((p) == NULL ? malloc_osi(n) : realloc_osi((p), (n)))
#else
#define remalloc(p,n)	realloc_osi((p), (n))
#endif

static struct lalloc_common *findptr(struct lalloc_common **, char *, Area *);

#ifndef MKSH_ALLOC_CATCH_UNDERRUNS
#define ALLOC_ISUNALIGNED(p) (((size_t)(p)) % sizeof(struct lalloc_common))
#else
#define ALLOC_ISUNALIGNED(p) (((size_t)(p)) & 4095)
#undef remalloc
#undef free_osimalloc

static void
free_osimalloc(void *ptr)
{
	struct lalloc_item *lp = ptr;

	if (munmap(lp, lp->len))
		err(1, "free_osimalloc");
}

static void *
remalloc(void *ptr, size_t size)
{
	struct lalloc_item *lp, *lold = ptr;

	size = (size_t)(size + 4095U) & (size_t)~(size_t)4095U;

	if (lold && lold->len >= size)
		return (ptr);

	if ((lp = mmap(NULL, size, PROT_READ | PROT_WRITE,
	    MAP_ANON | MAP_PRIVATE, -1, (off_t)0)) == MAP_FAILED)
		err(1, "remalloc: mmap(%zu)", size);
	if (ALLOC_ISUNALIGNED(lp))
		errx(1, "remalloc: unaligned(%p)", lp);
	if (mprotect(((char *)lp) + 4096, 4096, PROT_NONE))
		err(1, "remalloc: mprotect");
	lp->len = size;

	if (lold) {
		memcpy(((char *)lp) + 8192, ((char *)lold) + 8192,
		    lold->len - 8192);
		if (munmap(lold, lold->len))
			err(1, "remalloc: munmap");
	}

	return (lp);
}
#endif

/* pre-initio() */
void
ainit(Area *ap)
{
#ifdef MKSH_ALLOC_CATCH_UNDERRUNS
	if (sysconf(_SC_PAGESIZE) != 4096) {
		fprintf(stderr, "mksh: fatal: pagesize %lu not 4096!\n",
		    sysconf(_SC_PAGESIZE));
		fflush(stderr);
		abort();
	}
#endif
	/* area pointer and items share struct lalloc_common */
	ap->next = NULL;
}

static struct lalloc_common *
findptr(struct lalloc_common **lpp, char *ptr, Area *ap)
{
	void *lp;
#ifdef DEBUG
	Area *oap = ap;
#endif

#ifndef MKSH_SMALL
	if (ALLOC_ISUNALIGNED(ptr))
		goto fail;
#endif
	/* get address of ALLOC_ITEM from user item */
	/*
	 * note: the alignment of "ptr" to ALLOC_ITEM is checked
	 * above; the "void *" gets us rid of a gcc 2.95 warning
	 */
	*lpp = (lp = ptr - sizeof(ALLOC_ITEM));
	/* search for allocation item in group list */
	while (ap->next != lp)
		if ((ap = ap->next) == NULL) {
#ifndef MKSH_SMALL
 fail:
#endif
#ifdef DEBUG
			kwarnf0(KWF_INTERNAL | KWF_WARNING | KWF_NOERRNO,
			    "rogue pointer %zX in ap %zX",
			    (size_t)ptr, (size_t)oap);
			/* try to get a coredump */
			abort();
#else
			kerrf0(KWF_INTERNAL | KWF_ERR(0xFF) | KWF_NOERRNO,
			    "rogue pointer %zX", (size_t)ptr);
#endif
		}
	return (ap);
}

void *
aresize1(void *ptr, size_t len1, size_t len2, Area *ap)
{
	checkoktoadd(len1, len2);
	return (aresize(ptr, len1 + len2, ap));
}

/* pre-initio() */
void *
aresize2(void *ptr, size_t fac1, size_t fac2, Area *ap)
{
	if (notoktomul(fac1, fac2))
		kerrf0(KWF_INTERNAL | KWF_ERR(0xFF) | KWF_NOERRNO,
		    Tintovfl, fac1, '*', fac2);
	return (aresize(ptr, fac1 * fac2, ap));
}

/* pre-initio() ptr=NULL */
void *
aresize(void *ptr, size_t numb, Area *ap)
{
	struct lalloc_common *lp = NULL;

	/* resizing or newly allocating? */
	if (ptr != NULL) {
		struct lalloc_common *pp;

		pp = findptr(&lp, ptr, ap);
		pp->next = lp->next;
	}

	if (notoktoadd(numb, sizeof(ALLOC_ITEM))) {
		errno = E2BIG;
 alloc_fail:
		if (!initio_done) {
			SHIKATANAI write(2, SC("mksh: out of memory early\n"));
			exit(255);
		}
		kerrf0(KWF_INTERNAL | KWF_ERR(0xFF),
		    "can't allocate %zu data bytes", numb);
	}
	if ((lp = remalloc(lp, numb + sizeof(ALLOC_ITEM))) == NULL)
		goto alloc_fail;
#ifndef MKSH_SMALL
	if (ALLOC_ISUNALIGNED(lp)) {
#ifdef EPROTO
		errno = EPROTO;
#endif
		goto alloc_fail;
	}
#endif

	/* area pointer and items share struct lalloc_common */
	/*XXX C99 §6.5(6) and footnote 72 may dislike this? */
	lp->next = ap->next;
	ap->next = lp;
	/* return user item address */
	return ((char *)lp + sizeof(ALLOC_ITEM));
}

void
afree(void *ptr, Area *ap)
{
	if (ptr != NULL) {
		struct lalloc_common *lp, *pp;

		pp = findptr(&lp, ptr, ap);
		/* unhook */
		pp->next = lp->next;
		/* now free ALLOC_ITEM */
		free_osimalloc(lp);
	}
}

void
afreeall(Area *ap)
{
	struct lalloc_common *lp;

	/* traverse group (linked list) */
	while ((lp = ap->next) != NULL) {
		/* make next ALLOC_ITEM head of list */
		ap->next = lp->next;
		/* free old head */
		free_osimalloc(lp);
	}
}
