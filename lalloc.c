#include "sh.h"

__RCSID("$MirOS: src/bin/mksh/lalloc.c,v 1.1 2009/03/22 16:55:38 tg Exp $");

struct lalloc {
	struct lalloc *next;	/* entry pointer, must be first */
	Area *group;		/* group backpointer */
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
	if ((*lpp)->group != ap)
 notfound:
		internal_errorf("pointer %p not in group %p", ptr, ap);
	*ppp = (struct lalloc *)ap;
	while ((*ppp)->next != *lpp)
		if (((*ppp) = (*ppp)->next) == NULL)
			goto notfound;
}

void *
aresize(void *ptr, size_t numb, Area *ap)
{
	struct lalloc *lp, *pp;

	if (numb >= SIZE_MAX - sizeof (struct lalloc))
 failure:
		internal_errorf("cannot allocate %lu data bytes",
		    (unsigned long)numb);

	if (ptr == NULL) {
		pp = (struct lalloc *)ap;
		lp = malloc(numb + sizeof (struct lalloc));
	} else {
		findptr(&lp, &pp, ptr, ap);
		lp = realloc(lp, numb + sizeof (struct lalloc));
	}
	if (lp == NULL)
		goto failure;
	if (ptr == NULL) {
		lp->group = ap;
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
	/* lp->group = NULL; */
	free(lp);
}

void
afreeall(Area *ap)
{
	struct lalloc *lp;

	while ((lp = ap->ent) != NULL) {
#ifndef MKSH_SMALL
		if (lp->group != ap)
			internal_errorf("rogue pointer in group %p", ap);
#endif
		ap->ent = lp->next;
		/* lp->group = NULL; */
		free(lp);
	}
}
