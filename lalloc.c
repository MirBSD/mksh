#include "sh.h"

__RCSID("$MirOS: src/bin/mksh/lalloc.c,v 1.7 2009/03/24 18:34:39 tg Exp $");

#ifndef SIZE_MAX
#ifdef SIZE_T_MAX
#define SIZE_MAX	SIZE_T_MAX
#else
#define SIZE_MAX	((size_t)-1)
#endif
#endif

/* build with CPPFLAGS+= -DUSE_REALLOC_MALLOC=0 on ancient systems */
#if defined(USE_REALLOC_MALLOC) && (USE_REALLOC_MALLOC == 0)
#define remalloc(p,n)	((p) == NULL ? malloc(n) : realloc((p), (n)))
#else
#define remalloc(p,n)	realloc((p), (n))
#endif

static struct lalloc *findptr(struct lalloc **, char *, Area *);

void
ainit(Area *ap)
{
	ap->next = NULL;
}

static struct lalloc *
findptr(struct lalloc **lpp, char *ptr, Area *ap)
{
	*lpp = (struct lalloc *)(ptr - sizeof (struct lalloc));
	while (ap->next != *lpp)
		if ((ap = ap->next) == NULL)
			internal_errorf("rogue pointer %p", ptr);
	return (ap);
}

void *
aresize(void *ptr, size_t numb, Area *ap)
{
	struct lalloc *lp = NULL;

	if (ptr != NULL) {
		struct lalloc *pp;

		pp = findptr(&lp, ptr, ap);
		pp->next = lp->next;
	}

	if ((numb >= SIZE_MAX - sizeof (struct lalloc)) ||
	    (lp = remalloc(lp, numb + sizeof (struct lalloc))) == NULL)
		internal_errorf("cannot allocate %lu data bytes",
		    (unsigned long)numb);
	lp->next = ap->next;
	ap->next = lp;
	return ((char *)lp + sizeof (struct lalloc));
}

void
afree(void *ptr, Area *ap)
{
	if (ptr != NULL) {
		struct lalloc *lp, *pp;

		pp = findptr(&lp, ptr, ap);
		pp->next = lp->next;
		free(lp);
	}
}

void
afreeall(Area *ap)
{
	struct lalloc *lp;

	while ((lp = ap->next) != NULL) {
		ap->next = lp->next;
		free(lp);
	}
}
