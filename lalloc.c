#include "sh.h"

__RCSID("$MirOS: src/bin/mksh/lalloc.c,v 1.4 2009/03/24 08:37:37 tg Exp $");

#ifndef SIZE_MAX
#ifdef SIZE_T_MAX
#define SIZE_MAX	SIZE_T_MAX
#else
#define SIZE_MAX	((size_t)-1)
#endif
#endif

struct lalloc {
	struct lalloc *next;	/* entry pointer, must be first */
};

static void findptr(struct lalloc **, struct lalloc **, char *, Area *);

void
ainit(Area *ap)
{
	ap->ent = NULL;
}

static void
findptr(struct lalloc **lpp, struct lalloc **ppp, char *ptr, Area *ap)
{
	*lpp = (struct lalloc *)(ptr - sizeof (struct lalloc));
	*ppp = (struct lalloc *)ap;
	while ((*ppp)->next != *lpp)
		if (((*ppp) = (*ppp)->next) == NULL)
			internal_errorf("pointer %p not in group %p", ptr, ap);
}

void *
aresize(void *ptr, size_t numb, Area *ap)
{
	struct lalloc *lp, *pp;

	if (numb >= SIZE_MAX - sizeof (struct lalloc))
		goto failure;

	if (ptr == NULL) {
		pp = (struct lalloc *)ap;
		lp = malloc(numb + sizeof (struct lalloc));
	} else {
		findptr(&lp, &pp, ptr, ap);
		lp = realloc(lp, numb + sizeof (struct lalloc));
	}
	if (lp == NULL) {
 failure:
		internal_errorf("cannot allocate %lu data bytes",
		    (unsigned long)numb);
	}
	if (ptr == NULL) {
		lp->next = ap->ent;
	}
	pp->next = lp;
	return ((char *)lp + sizeof (struct lalloc));
}

void
afree(void *ptr, Area *ap)
{
	struct lalloc *lp, *pp;

	if (ptr == NULL)
		return;

	findptr(&lp, &pp, ptr, ap);
	pp->next = lp->next;
	free(lp);
}

void
afreeall(Area *ap)
{
	struct lalloc *lp;

	while ((lp = ap->ent) != NULL) {
		ap->ent = lp->next;
		free(lp);
	}
}
