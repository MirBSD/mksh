#include "sh.h"

__RCSID("$MirOS: src/bin/mksh/lalloc.c,v 1.9 2009/04/07 18:56:51 tg Exp $");

/* build with CPPFLAGS+= -DUSE_REALLOC_MALLOC=0 on ancient systems */
#if defined(USE_REALLOC_MALLOC) && (USE_REALLOC_MALLOC == 0)
#define remalloc(p,n)	((p) == NULL ? malloc(n) : realloc((p), (n)))
#else
#define remalloc(p,n)	realloc((p), (n))
#endif

static ALLOC_ITEM *findptr(ALLOC_ITEM **, char *, Area *);

void
ainit(Area *ap)
{
	/* area pointer is an ALLOC_ITEM, just the head of the list */
	ap->next = NULL;
}

static ALLOC_ITEM *
findptr(ALLOC_ITEM **lpp, char *ptr, Area *ap)
{
	/* get address of ALLOC_ITEM from user item */
	*lpp = (ALLOC_ITEM *)(ptr - ALLOC_SIZE);
	/* search for allocation item in group list */
	while (ap->next != *lpp)
		if ((ap = ap->next) == NULL)
			internal_errorf("rogue pointer %p", ptr);
	return (ap);
}

void *
aresize(void *ptr, size_t numb, Area *ap)
{
	ALLOC_ITEM *lp = NULL;

	/* resizing (true) or newly allocating? */
	if (ptr != NULL) {
		ALLOC_ITEM *pp;

		pp = findptr(&lp, ptr, ap);
		pp->next = lp->next;
	}

	if ((numb >= SIZE_MAX - ALLOC_SIZE) ||
	    (lp = remalloc(lp, numb + ALLOC_SIZE)) == NULL)
		internal_errorf("cannot allocate %lu data bytes",
		    (unsigned long)numb);
	/* this only works because Area is an ALLOC_ITEM */
	lp->next = ap->next;
	ap->next = lp;
	/* return user item address */
	return ((char *)lp + ALLOC_SIZE);
}

void
afree(void *ptr, Area *ap)
{
	if (ptr != NULL) {
		ALLOC_ITEM *lp, *pp;

		pp = findptr(&lp, ptr, ap);
		/* unhook */
		pp->next = lp->next;
		/* now free ALLOC_ITEM */
		free(lp);
	}
}

void
afreeall(Area *ap)
{
	ALLOC_ITEM *lp;

	/* traverse group (linked list) */
	while ((lp = ap->next) != NULL) {
		/* make next ALLOC_ITEM head of list */
		ap->next = lp->next;
		/* free old head */
		free(lp);
	}
}
