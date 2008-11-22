/*-
 * group-based allocation manager based on (mmap) malloc
 * Copyright (c) 2008
 *	Thorsten "mirabilos" Glaser <tg@mirbsd.org>
 */

/** Configurables:
 * GALLOC_ABORT		printf-style dead function
 * GALLOC_DEBUG		def/undef: more debugging, no cookies
 * GALLOC_HINT		suggested initial list malloc size (in pointers)
 * GALLOC_INTERR	use internal ABORT and WARN functions
 * GALLOC_LEAK_WARN	def/undef: warn if leaking on exit
 * GALLOC_MPROTECT	use mprotect(2)
 * GALLOC_NO_COOKIES	def/undef: disable cookies
 * GALLOC_RANDOM	arc4random-style function
 * GALLOC_SMALL		def/undef: disable debug, stats, track, etc.
 * GALLOC_STATS		def/undef: enable track, print size stats
 * GALLOC_THREAD_ENTER	@synchronised(foo) {
 * GALLOC_THREAD_LEAVE	}
 * GALLOC_TRACK		def/undef: track everything in a root group
 * GALLOC_WARN		printf-style function
 **
 * If you have any thread locking initialisation to do, such as allocation
 * of a mutex object, you must do it in the integration part.
 * Define GALLOC_H to get the interface only
 */

#ifndef GALLOC_H

/* galloc: mksh integration */

#include "sh.h"

#define GALLOC_INTERR	/* for now */

#ifndef GALLOC_ABORT
#define GALLOC_ABORT		internal_errorf
#endif

#ifndef GALLOC_WARN
#define GALLOC_WARN		internal_warningf
#endif

#ifndef GALLOC_RANDOM
#if HAVE_ARC4RANDOM
#define GALLOC_RANDOM()		arc4random()
#else
#ifndef RAND_MAX
#define RAND_MAX		0x7FFF
#endif
#define GALLOC_RANDOM()		((rand() % (RAND_MAX + 1)) * (RAND_MAX + 1) + \
				 (rand() % (RAND_MAX + 1)))
#endif
#endif

#else /* GALLOC_H */

#ifdef DEBUG
#undef GALLOC_DEBUG
#define GALLOC_DEBUG
#endif
#ifdef MKSH_SMALL
#undef GALLOC_SMALL
#define GALLOC_SMALL
#endif

#endif

/* galloc: implementation */

#ifdef GALLOC_H

#ifdef GALLOC_SMALL
#undef GALLOC_DEBUG
#undef GALLOC_STATS
#undef GALLOC_TRACK
#undef GALLOC_NO_COOKIES
#define GALLOC_NO_COOKIES
#endif
#ifdef GALLOC_STATS
#undef GALLOC_TRACK
#define GALLOC_TRACK
#endif

#ifdef GALLOC_STATS
#define GALLOC_VST(x)		, x
#define GALLOC_VSTx(x)		x
#else
#define GALLOC_VST(x)		/* nothing */
#define GALLOC_VSTx(x)		/* nothing */
#endif

typedef ptrdiff_t TCookie;

typedef union {
	ptrdiff_t iv;		/* integral (cooked) value */
#if defined(GALLOC_DEBUG) && defined(GALLOC_NO_COOKIES)
	void *pv;		/* pointer (uncooked), for gdb inspection */
#endif
} TCooked;

typedef union {
	void *fwp;		/* address of the allocation */
	TCooked *bwp;		/* address of the backpointer */
} TEntry;

typedef struct {
	TCooked bp;		/* [void **] backpointer */
	TCooked ap;		/* [PList] allocations */
	TCooked cp;		/* [PList] child groups */
#if defined(GALLOC_TRACK) && !defined(GALLOC_NO_COOKIES)
	TCooked sc;		/* [TCookie] saved list cookie */
#endif
#ifdef GALLOC_STATS
	const char *name;
	ptrdiff_t numaent;
	ptrdiff_t numcent;
#endif
} TGroup;
typedef TGroup *PGroup;

typedef struct {
#ifndef GALLOC_NO_COOKIES
	TCookie lc;		/* list cookie */
#endif
	TEntry *ep;		/* pointer to end of storage area */
	TEntry *up;		/* pointer to first unused storage */
	TEntry ntr;		/* first entry (storage area) */
} TList;
typedef TList *PList;

PGroup galloc_new(PGroup, PGroup, size_t  GALLOC_VST(const char *));
void *galloc(size_t, size_t, PGroup);
void *grealloc(void *, size_t, size_t, PGroup);
void gfree(void *, PGroup);
void galloc_del(PGroup);

#else /* !GALLOC_H */

__RCSID("$MirOS: src/bin/mksh/galloc.c,v 1.1.2.1 2008/11/22 13:20:30 tg Exp $");

#ifndef GALLOC_HINT
#define GALLOC_HINT		32
#endif

#define PVSIZE			(sizeof (void *))
#define PVMASK			((ptrdiff_t)(sizeof (void *) - 1))
#define isunaligned(p)		(((ptrdiff_t)(p) & PVMASK) != 0)
#define ispoweroftwo(n)		((unsigned long)(n) == (1UL << (ffs(n) - 1)))
#define galloc_listsz(lp)	((char *)(lp)->ep - (char *)(lp))

#ifdef GALLOC_NO_COOKIES
#define iscook(c)		true
#define ldcook(i, c)		((void *)(i).iv)
#define stcook(i, p, c)		(i).iv = (ptrdiff_t)(p)
#else
#define iscook(c)		isunaligned(c)
#define ldcook(i, c)		((void *)((i).iv ^ (c)))
#define stcook(i, p, c)		(i).iv = ((ptrdiff_t)(p)) ^ (c)
#endif
#define zfcook(i)		((i).iv == 0)
#define zrcook(i)		(i).iv = 0

#ifdef GALLOC_INTERR
#undef GALLOC_ABORT
#undef GALLOC_WARN
#define GALLOC_ABORT(...)	galloc_interr(true, __VA_ARGS__)
#define GALLOC_WARN(...)	galloc_interr(false, __VA_ARGS__)
#endif

#ifdef GALLOC_MPROTECT
#undef GALLOC_MPROTECT
#define GALLOC_MPROTECT(usecp, p, sz, mode) do {	\
	if (!(usecp))					\
		mprotect((p), (sz), (mode));		\
} while (/* CONSTCOND */ 0)
#define GALLOC_ALLOW(u, p, sz)	GALLOC_MPROTECT((u), (p), (sz), \
				    PROT_READ | PROT_WRITE)
#define GALLOC_DENY(u, p, sz)	GALLOC_MPROTECT((u), (p), (sz), PROT_NONE)
#else
#define GALLOC_ALLOW(u, p, sz)	/* nothing */
#define GALLOC_DENY(u, p, sz)	/* nothing */
#endif

#ifndef GALLOC_THREAD_ENTER
#define GALLOC_THREAD_ENTER	/* nothing */
#endif
#ifndef GALLOC_THREAD_LEAVE
#define GALLOC_THREAD_LEAVE	/* nothing */
#endif

#ifdef GALLOC_SMALL
#define GALLOC_VSM(x, ...)	x
#define GALLOC_FUNC(x)		/* nothing */
#else
#define GALLOC_VSM(x, ...)	__VA_ARGS__
#define GALLOC_FUNC(x)		Tf = (x);
#endif

static TCookie gc;

static const char T_ECONT[] = "cannot continue";
static const char T_ENOMEM[] = "cannot allocate memory";
static const char Tf_xrealloc[] = "xrealloc";
#ifdef GALLOC_TRACK
static const char Tf_galloc_atexit[] = "galloc_atexit";
#endif
#ifndef GALLOC_SMALL
static const char Tf_galloc_del[] = "galloc_del";
static const char Tf_galloc_list_access[] = "galloc_list_access";
static const char Tf_galloc_list_find[] = "galloc_list_find";
static const char Tf_gfree[] = "gfree";
static const char Tf_grealloc[] = "grealloc";
static const char *Tf;
static const char T_corrupt[] = "%s: %s "
    GALLOC_VSTx("%s(") "%p" GALLOC_VSTx(")") " corrupt: %s = %p";
static const char Tm_group[] = "group";
static const char T_rogue[] = "%s: trying to %s rogue %s pointer %p <- %p";
#else
static const char T_EBOGUS[] = "bogus pointer %p";
static const char T_EINVAL[] = "invalid control data";
#endif
#ifdef GALLOC_DEBUG
static const char Tf_galloc[] = "galloc";
static const char Tf_galloc_new[] = "galloc_new";
static const char Tf_galloc_setup[] = "galloc_setup";
static const char T_ENULL[] = "%s: %s == NULL";
static const char T_pow2[] = "%s: %s (%lu) is not a power of two";
#endif

#ifdef GALLOC_MPROTECT
#undef GALLOC_HINT
#define GALLOC_HINT		(pagesz / PVSIZE)
static long pagesz;
#endif

#ifdef GALLOC_TRACK
static TGroup rg;		/* root group */
static void galloc_atexit(void);
#endif

#ifdef GALLOC_INTERR
static void galloc_interr(bool, const char *, ...)
    __attribute__((format (printf, 2, 3)));
#endif
#ifdef GALLOC_STATS
static void galloc_puts(const char *, ...)
    __attribute__((format (printf, 1, 2)));
#endif

static void galloc_dispose(PGroup);
static char *galloc_get(size_t, size_t, PGroup, bool);
static PList galloc_list_access(PGroup, bool);
static void galloc_list_enter(void *, PList *, TCooked *
    GALLOC_VST(ptrdiff_t *));
static TEntry *galloc_list_find(char *, PList *, PGroup);
static PList galloc_list_new(size_t);
static void galloc_setup(void);
static void *xrealloc(void *, size_t, size_t *, size_t);

#ifdef GALLOC_INTERR
static void
galloc_interr(bool abend, const char *fmt, ...)
{
	va_list ap;

	fflush(NULL);
	fprintf(stderr, "galloc_abort: ");
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	putc('\n', stderr);
	fflush(NULL);

	if (abend)
		exit(255);
}
#endif

#ifdef GALLOC_STATS
/**
 * helper function for outputting statistics
 */
static void
galloc_puts(const char *fmt, ...)
{
	va_list ap;
	FILE *of;
	static int gpid = 0;

	if (!gpid)
		gpid = (int)getpid();

	va_start(ap, fmt);
	if ((of = fopen("/tmp/galloc.out", "ab+"))) {
		fprintf(of, "%08X %5d: ", (unsigned int)time(NULL), gpid);
		vfprintf(of, fmt, ap);
		putc('\n', of);
		fclose(of);
	} else
		internal_verrorf(fmt, ap);
	va_end(ap);
}
#endif

/**
 * safe malloc/realloc (aligned, cannot fail, cannot overflow)
 * - optr:	old pointer, or NULL for malloc
 * - nmemb:	number of members to allocate
 * - psiz:	in: size of a member
 *		out: total allocated size (nmemb * *psiz + extra)
 * - extra:	extra bytes to allocate
 * returns:	PV aligned storage of out:*psiz bytes
 */
static void *
xrealloc(void *optr, size_t nmemb, size_t *psiz, size_t extra)
{
	void *ptr;
	int en;

	if ((nmemb >= (1UL << (sizeof (size_t) * 8 / 2)) ||
	    (*psiz) >= (1UL << (sizeof (size_t) * 8 / 2))) &&
	    nmemb > 0 && (SIZE_MAX / nmemb < (*psiz)))
		GALLOC_ABORT(GALLOC_VSM(T_ENOMEM,
		    "%s: attempted integer overflow: %lu * %lu", Tf_xrealloc,
		    (unsigned long)nmemb, (unsigned long)*psiz));
	*psiz *= nmemb;
	if (*psiz >= SIZE_MAX - extra)
		GALLOC_ABORT(GALLOC_VSM(T_ENOMEM,
		    "%s: not enough extra space for %lu in %lu", Tf_xrealloc,
		    (unsigned long)extra, (unsigned long)*psiz));
	*psiz += extra;

	ptr = optr ? malloc(*psiz) : realloc(optr, *psiz);
	en = errno;
#ifdef GALLOC_DEBUG
	if (ptr == NULL)
		GALLOC_ABORT("%s: %s (%lu bytes): %s", Tf_xrealloc,
		    T_ENOMEM, (unsigned long)*psiz, strerror(en));
	if (isunaligned(ptr))
		GALLOC_ABORT("%s: unaligned malloc: %p", Tf_xrealloc, ptr);
#else
	if (isunaligned(ptr))
		GALLOC_ABORT("%s: %s (%lu -> %p): %s", Tf_xrealloc,
		    T_ENOMEM, (unsigned long)*psiz, ptr, strerror(en));
#endif
	return (ptr);
}

/**
 * first-time setup function
 */
static void
galloc_setup(void)
{
#ifndef GALLOC_NO_COOKIES
	size_t v;
#endif
#ifdef GALLOC_TRACK
	PList lp;
#endif

#ifdef GALLOC_MPROTECT
	if ((pagesz = sysconf(_SC_PAGESIZE)) == -1 ||
	    (size_t)(pagesz / PVSIZE) * PVSIZE != (size_t)pagesz ||
#ifdef GALLOC_DEBUG
	    !ispoweroftwo(pagesz) ||
#endif
	    (size_t)pagesz < (size_t)PVSIZE)
		GALLOC_ABORT("sysconf(_SC_PAGESIZE) failed: %ld %s",
		    pagesz, strerror(errno));
#endif

#ifdef GALLOC_DEBUG
	if (!ispoweroftwo(PVSIZE))
		GALLOC_ABORT(T_pow2, Tf_galloc_setup, "PVSIZE",
		    (unsigned long)PVSIZE);
	if (!ispoweroftwo(GALLOC_HINT))
		GALLOC_ABORT(T_pow2, Tf_galloc_setup, "GALLOC_HINT",
		    (unsigned long)GALLOC_HINT);
	if (sizeof (ptrdiff_t) != sizeof(TCookie) ||
	    sizeof (ptrdiff_t) != sizeof(TCooked) ||
	    sizeof (ptrdiff_t) != PVSIZE ||
	    sizeof (ptrdiff_t) != sizeof(TEntry))
		GALLOC_ABORT("data type sizes mismatch: %lu %lu %lu %lu %lu",
		    (unsigned long)sizeof (ptrdiff_t),
		    (unsigned long)sizeof (TCookie),
		    (unsigned long)sizeof (TCooked),
		    (unsigned long)PVSIZE,
		    (unsigned long)sizeof (TEntry));
	if ((size_t)GALLOC_HINT * PVSIZE < sizeof (TList))
		GALLOC_ABORT("%s: %s (%lu) too small: < %lu",
		    Tf_galloc_setup, "GALLOC_HINT",
		    (unsigned long)(GALLOC_HINT * PVSIZE),
		    (unsigned long)sizeof (TList));
#endif

#ifdef GALLOC_NO_COOKIES
	gc = 1;
#else
	/* ensure we get an unaligned cookie and randomise */
	v = 0;
	do {
		gc = GALLOC_RANDOM();
		v |= GALLOC_RANDOM() << 3;
		if (v & 8)
			v >>= 4;
		v &= 7;
	} while (!iscook(gc) || !v);
	while (--v)
		(void)GALLOC_RANDOM();
#endif
#ifdef GALLOC_TRACK
	lp = galloc_list_new(8);
#ifndef GALLOC_NO_COOKIES
	lp->lc = gc;
#endif
	zrcook(rg.bp);
	zrcook(rg.ap);
	stcook(rg.cp, lp, gc);
#ifndef GALLOC_NO_COOKIES
	zrcook(rg.sc);
#endif
#ifdef GALLOC_STATS
	rg.name = "root group";
	rg.numaent = 0;
	rg.numcent = 0;
#endif
	atexit(galloc_atexit);
#endif
}

/**
 * Allocate and initialise a list
 */
static PList
galloc_list_new(size_t hint)
{
	PList lp;

	lp = xrealloc(NULL, PVSIZE, &hint, 0);
#ifndef GALLOC_NO_COOKIES
	do {
		lp->lc = GALLOC_RANDOM();
	} while (!iscook(lp->lc));
#endif
	lp->ep = (TEntry *)((char *)lp + hint);
	lp->up = &lp->ntr;
	return (lp);
}

/**
 * validate a group, access one of its lists
 * - gp:	group to validate and use
 * - usecp:	if true: child list, else: allocation list
 * Note: the list must already exist; it is unlocked and its address returned
 */
static PList
galloc_list_access(PGroup gp, bool usecp)
{
	PList lp = NULL;
	TCooked *tc;
#ifndef GALLOC_SMALL
	const char *tn;
#endif

#ifndef GALLOC_SMALL
	if (!zfcook(gp->bp) && isunaligned(ldcook(gp->bp, gc))) {
		GALLOC_WARN(T_corrupt, Tf_galloc_list_access, Tm_group
		    GALLOC_VST(gp->name), gp, "bp", ldcook(gp->bp, gc));
		return (NULL);
	}
#if defined(GALLOC_TRACK) && !defined(GALLOC_NO_COOKIES)
	if (!zfcook(gp->sc) && !iscook(ldcook(gp->sc, gc))) {
		GALLOC_WARN(T_corrupt, Tf_galloc_list_access, Tm_group
		    GALLOC_VST(gp->name), gp, "sc", ldcook(gp->sc, gc));
		return (NULL);
	}
#endif
	tn = usecp ? "cp" : "ap";
#endif
	tc = usecp ? &gp->cp : &gp->ap;
	if (zfcook(*tc) || isunaligned(lp = ldcook(*tc, gc))) {
		GALLOC_WARN(GALLOC_VSM(T_EINVAL, T_corrupt,
		    Tf_galloc_list_access, Tm_group
		    GALLOC_VST(gp->name), gp, tn, lp));
		return (NULL);
	}
	GALLOC_ALLOW(usecp, lp, sizeof (TList));
#ifndef GALLOC_SMALL
#if defined(GALLOC_TRACK) && !defined(GALLOC_NO_COOKIES)
	if (lp->lc != (TCookie)ldcook(gp->sc, gc))
		GALLOC_WARN(T_corrupt, Tf_galloc_list_access, tn
		    GALLOC_VST(gp->name), gp, "lc", (void *)lp->lc);
	else
#endif
	  if (isunaligned(lp->ep))
		GALLOC_WARN(T_corrupt, Tf_galloc_list_access, tn
		    GALLOC_VST(gp->name), gp, "ep", lp->ep);
	else if (isunaligned(lp->up))
		GALLOC_WARN(T_corrupt, Tf_galloc_list_access, tn
		    GALLOC_VST(gp->name), gp, "up", lp->up);
	else if (lp->up < &lp->ntr || lp->up > lp->ep)
		GALLOC_WARN("%s: %s " GALLOC_VSTx("%s(") "%p" GALLOC_VSTx(")")
		    " out of bounds: %p < %p < %p", Tf_galloc_list_access, tn
		    GALLOC_VST(gp->name), gp, &lp->ntr, lp->up, lp->ep);
	else {
#endif
		GALLOC_ALLOW(usecp, lp, galloc_listsz(lp));
		return (lp);
#ifndef GALLOC_SMALL
	}
	GALLOC_DENY(usecp, lp, sizeof (TList));
	return (NULL);
#endif
}

/**
 * enter a newly allocated pointer into a list
 * - ptr:	pointer to the allocation’s backpointer
 * - plp:	pointer to the list address, potentially modified
 * - ra:	pointer to the list cooked, modified on list resize
 * - pacct:	pointer to the list accounting (GALLOC_STATS)
 */
static void
galloc_list_enter(void *ptr, PList *plp, TCooked *ra
    GALLOC_VST(ptrdiff_t *pacct))
{
	PList lp = *plp;
	TEntry *np;
#ifdef GALLOC_STATS
	ptrdiff_t pd;
#endif

	/* check if we need to resize the list */
	if (lp->up == lp->ep) {
		size_t sz;

		sz = galloc_listsz(lp);
		lp = xrealloc(lp, 2, &sz, 0);
		lp->ep = (TEntry *)((char *)lp + sz);
		lp->up = (TEntry *)((char *)lp + (sz / 2));
		*plp = lp;
		stcook(*ra, lp, gc);

		/* adjust all backpointers */
		for (np = &lp->ntr; np < lp->up; ++np)
			stcook(*(np->bwp), np, lp->lc);
	}

	/* write to lp->up and increment it */
	np = lp->up++;
	np->fwp = ptr;
	stcook(*(np->bwp), np, lp->lc);

#ifdef GALLOC_STATS
	pd = (char *)lp->up - (char *)lp;
	pd /= PVSIZE;
	if (*pacct < pd)
		*pacct = pd;
#endif
}

/**
 * open and validate allocation list, find and validate entry for
 * user pointer ptr (offset 4 to malloc), return address of TEntry
 * and unlocked list pointer in *plp, or NULL with list locked
 * + Tf:	(global) what are we trying to do
 */
static TEntry *
galloc_list_find(char *ptr, PList *plp, PGroup gp)
{
	TEntry *ep = NULL;
	TCooked *kp;

	if (isunaligned(ptr)) {
		GALLOC_WARN(GALLOC_VSM(T_EBOGUS, T_rogue, Tf_galloc_list_find,
		    Tf, "unaligned", NULL), ptr);
		return (NULL);
	}

	if ((*plp = galloc_list_access(gp, false)) == NULL) {
		GALLOC_WARN(GALLOC_VSM(T_EINVAL, "%s: invalid group in %s",
		    Tf_galloc_list_find, Tf));
		return (NULL);
	}

	kp = (TCooked *)(ptr - PVSIZE);
	if (zfcook(*kp) || isunaligned(ep = ldcook(*kp, (*plp)->lc)))
		GALLOC_WARN(GALLOC_VSM(T_EBOGUS, T_rogue, Tf_galloc_list_find,
		    Tf, "underflown", ep), ptr);
	else if (ep < &(*plp)->ntr || ep > (*plp)->up)
		GALLOC_WARN(GALLOC_VSM(T_EBOGUS, "%s: group " GALLOC_VSTx("%s(")
		    "%p" GALLOC_VSTx(")") ", trying to %s rogue out of bounds "
		    "(%p < %p < %p) pointer %p", Tf_galloc_list_find
		    GALLOC_VST(gp->name), gp, Tf, &(*plp)->ntr, ep,
		    (*plp)->up), ptr);
	else if (ep->fwp != kp) {
#ifndef GALLOC_SMALL
		GALLOC_WARN(T_corrupt, Tf_galloc_list_find, "pointer"
		    GALLOC_VST(gp->name), gp, "fwdptr", ep->fwp);
#endif
		GALLOC_WARN(GALLOC_VSM(T_EBOGUS, T_rogue, Tf_galloc_list_find,
		    Tf, "mismatching", ep), ptr);
	} else
		return (ep);

	/* error case */
	GALLOC_DENY(false, *plp, galloc_listsz(*plp));
	return (NULL);
}

/**
 * initialise a new group
 * - gp:	address of static storage to use, or NULL
 * - parent:	parent group to allocate dynamic storage from, or ignored
 * - hint:	initial size of the list allocation, in pointers, or 0
 * - name:	friendly name to store for the group (GALLOC_STATS)
 * returns:	constant address of initialised group
 */
PGroup
galloc_new(PGroup gp, PGroup parent, size_t hint
    GALLOC_VST(const char *name))
{
	PList lp;

	GALLOC_THREAD_ENTER
	if (!gc)
		galloc_setup();

#ifdef GALLOC_DEBUG
	if (!ispoweroftwo(hint))
		GALLOC_ABORT(T_pow2, Tf_galloc_new, "hint",
		    (unsigned long)hint);
#endif

	if (gp) {
#ifdef GALLOC_TRACK
		if ((lp = galloc_list_access(&rg, true)) == NULL)
			GALLOC_ABORT("GALLOC_TRACK data structure (root group "
			    "child list) destroyed, aborting");
		galloc_list_enter(gp, &lp, &rg.cp  GALLOC_VST(&rg.numcent));
#else
		zrcook(gp->bp);
#endif
	} else {
#ifdef GALLOC_DEBUG
		if (!parent)
			GALLOC_ABORT(T_ENULL, Tf_galloc_new, "parent");
#endif
		if (zfcook(parent->cp)) {
			lp = galloc_list_new(8);
#ifndef GALLOC_NO_COOKIES
			lp->lc = gc;
#endif
			stcook(parent->cp, lp, gc);
		}
		gp = (PGroup)galloc_get(1, sizeof (TGroup), parent, true);
	}
#ifdef GALLOC_MPROTECT
	if (hint < GALLOC_HINT)
#else
	if (hint < sizeof (TList) / PVSIZE)
#endif
		hint = GALLOC_HINT;
	lp = galloc_list_new(hint);
	stcook(gp->ap, lp, gc);
	zrcook(gp->cp);
#if defined(GALLOC_TRACK) && !defined(GALLOC_NO_COOKIES)
	stcook(gp->sc, lp->lc, gc);
#endif
#ifdef GALLOC_STATS
	gp->name = name ? name : "(null)";
	gp->numaent = 0;
	gp->numcent = 0;
#endif

	GALLOC_DENY(false, lp, galloc_listsz(lp));
	GALLOC_THREAD_LEAVE
	return (gp);
}

/**
 * allocate (nmemb * siz) bytes of user-visible memory in group gp
 * with overflow checking, cannot fail
 */
void *
galloc(size_t nmemb, size_t siz, PGroup gp)
{
	char *ptr;

	GALLOC_THREAD_ENTER
#ifdef GALLOC_DEBUG
	if (!gp)
		GALLOC_ABORT(T_ENULL, Tf_galloc, "gp");
#endif

	ptr = galloc_get(nmemb, siz, gp, false) + PVSIZE;
	GALLOC_THREAD_LEAVE
	return (ptr);
}

/**
 * allocate (nmemb * siz) bytes of user-visible memory in the child (if
 * usecp is true) or allocation (if usecp is false) list of group gp,
 * with overflow checking, cannot fail; return pointer to backpointer
 */
static char *
galloc_get(size_t nmemb, size_t siz, PGroup gp, bool usecp)
{
	PList lp;
	char *ptr;

	if ((lp = galloc_list_access(gp, usecp)) == NULL)
		GALLOC_ABORT(T_ECONT);
	ptr = xrealloc(NULL, nmemb, &siz, PVSIZE);
	galloc_list_enter(ptr, &lp, usecp ? &gp->cp : &gp->ap
	    GALLOC_VST(usecp ? &gp->numcent : &gp->numaent));
	GALLOC_DENY(usecp, lp, galloc_listsz(lp));

	return (ptr);
}

void *
grealloc(void *ptr, size_t nmemb, size_t siz, PGroup gp)
{
	TEntry *ep;
	PList lp;
	char *rv;

	if (ptr == NULL)
		return (galloc(nmemb, siz, gp));

	GALLOC_THREAD_ENTER
	GALLOC_FUNC(Tf_grealloc)
#ifdef GALLOC_DEBUG
	if (!gp)
		GALLOC_ABORT(T_ENULL, Tf_grealloc, "gp");
#endif

	if ((ep = galloc_list_find(ptr, &lp, gp)) == NULL)
		GALLOC_ABORT(T_ECONT);
	rv = ep->fwp = xrealloc(ep->fwp, nmemb, &siz, PVSIZE);
	GALLOC_DENY(false, lp, galloc_listsz(lp));
	GALLOC_FUNC(NULL)
	GALLOC_THREAD_LEAVE
	return (rv + PVSIZE);
}

void
gfree(void *ptr, PGroup gp)
{
	TEntry *ep;
	PList lp;
	char *rv;

	if (ptr == NULL)
		return;

	GALLOC_THREAD_ENTER
	GALLOC_FUNC(Tf_gfree)
#ifdef GALLOC_DEBUG
	if (!gp)
		GALLOC_ABORT(T_ENULL, Tf_grealloc, "gp");
#endif

	if ((ep = galloc_list_find(ptr, &lp, gp)) != NULL) {
		rv = ep->fwp;
		zrcook(*(ep->bwp));
		--lp->up;
		if (ep != lp->up) {
			ep->fwp = lp->up->fwp;
			stcook(*(ep->bwp), ep, lp->lc);
		}
		free(rv);
		GALLOC_DENY(false, lp, galloc_listsz(lp));
	}

	GALLOC_FUNC(NULL)
	GALLOC_THREAD_LEAVE
}

void
galloc_del(PGroup gp)
{
	GALLOC_THREAD_ENTER
	GALLOC_FUNC(Tf_galloc_del)
#ifdef GALLOC_DEBUG
	if (!gp)
		GALLOC_ABORT(T_ENULL, Tf_galloc_del, "gp");
#endif

	galloc_dispose(gp);

	GALLOC_FUNC(NULL)
	GALLOC_THREAD_LEAVE
}

#ifdef GALLOC_TRACK
static void
galloc_atexit(void)
{
	GALLOC_THREAD_ENTER
	GALLOC_FUNC(Tf_galloc_atexit)
	galloc_dispose(&rg);
	GALLOC_FUNC(NULL)
	GALLOC_THREAD_LEAVE
}
#endif

static void
galloc_dispose(PGroup gp __unused)
{
}

#endif /* !GALLOC_H */
