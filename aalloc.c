#include "sh.h"

__RCSID("$MirOS: src/bin/mksh/aalloc.c,v 1.31 2008/11/15 11:42:18 tg Exp $");

/* mksh integration of aalloc */

#if defined(AALLOC_STATS) && !defined(AALLOC_WARN)
#define AALLOC_WARN		aalloc_warn
static void aalloc_warn(const char *, ...)
    __attribute__((format (printf, 1, 2)));
static void
aalloc_warn(const char *fmt, ...)
{
	va_list va;
	FILE *of;

	va_start(va, fmt);
	if ((of = fopen("/tmp/aalloc.out", "ab+"))) {
		fprintf(of, "%08X %5d: ", (unsigned int)time(NULL),
		    (int)getpid());
		vfprintf(of, fmt, va);
		putc('\n', of);
		fclose(of);
	} else
		internal_verrorf(fmt, va);
}
#endif

#ifndef AALLOC_ABORT
#define AALLOC_ABORT		internal_errorf
#endif

#ifndef AALLOC_WARN
#define AALLOC_WARN		internal_warningf
#endif

#ifndef AALLOC_THREAD_ENTER
#define AALLOC_THREAD_ENTER(ap)	/* nothing */
#define AALLOC_THREAD_LEAVE(ap)	/* nothing */
#endif

#ifndef AALLOC_LEAK_SILENT
#define AALLOC_LEAK_SILENT	/* the code does not yet clean up at exit */
#endif

#ifndef AALLOC_RANDOM
#if HAVE_ARC4RANDOM
#define AALLOC_RANDOM()		arc4random()
#else
#define AALLOC_RANDOM()		(rand() * RAND_MAX + rand())
#endif
#endif

#undef AALLOC_SMALL
#ifdef MKSH_SMALL
#define AALLOC_SMALL		/* skip sanity checks */
#endif

#if defined(DEBUG) && !defined(AALLOC_DEBUG)
#define AALLOC_DEBUG		/* add extra sanity checks */
#endif

/* generic area-based allocator built for mmap malloc or omalloc */

#if defined(AALLOC_SMALL)
#undef AALLOC_DEBUG
#undef AALLOC_STATS
#undef AALLOC_TRACK
#ifndef AALLOC_NO_COOKIES
#define AALLOC_NO_COOKIES
#endif
#elif defined(AALLOC_STATS) && !defined(AALLOC_TRACK)
#define AALLOC_TRACK
#endif

#define PVALIGN			(sizeof (void *))
#define PVMASK			(sizeof (void *) - 1)
#define isunaligned(p)		(((ptrdiff_t)(p) & PVMASK) != 0)

#ifndef AALLOC_INITSZ
#define AALLOC_INITSZ		(64 * PVALIGN)	/* at least 4 pointers */
#endif

typedef /* unsigned */ ptrdiff_t TCookie;
typedef union {
	/* unsigned */ ptrdiff_t xv;
} TCooked;

typedef union {
	ptrdiff_t nv;
	char *cp;
} TPtr;

#ifdef AALLOC_NO_COOKIES
#define ldcook(p, cv, c)	((p).nv = (cv).xv, (p).nv)
#define stcook(cv, p, c)	((cv).xv = (p).nv, (p).nv)
#define stcookp(cv, p, c)	(xp.cp = (char *)(p), (cv).xv = xp.nv, xp.nv)
#define stcooki(cv, i, c)	((cv).xv = (xp.nv = (i)), xp.nv)
#define iscook(c)		true
#else
#define ldcook(p, cv, c)	((p).nv = (cv).xv ^ (c), (p).nv)
#define stcook(cv, p, c)	((cv).xv = (p).nv ^ (c), (p).nv)
#define stcookp(cv, p, c)	(xp.cp = (char *)(p), (cv).xv = xp.nv ^ (c), xp.nv)
#define stcooki(cv, i, c)	((cv).xv = (xp.nv = (i)) ^ (c), xp.nv)
#define iscook(c)		isunaligned(c)
#endif

/*
 * The separation between TBlock and TArea does not seem to make
 * sense at first, especially in the !AALLOC_TRACK case, but is
 * necessary to keep PArea values constant even if the storage is
 * enlarged. While we could use an extensible array to keep track
 * of the TBlock instances, kind of like we use TBlock.storage to
 * track the allocations, it would require another TBlock member
 * and a fair amount of backtracking; since omalloc can optimise
 * pointer sized allocations like a !AALLOC_TRACK TArea, we don't
 * do that then.
 */

struct TBlock {
#ifndef AALLOC_NO_COOKIES
	TCookie bcookie;
#endif
	char *endp;
	char *last;
	void *storage;
};
typedef struct TBlock *PBlock;

struct TArea {
	TCooked bp;
#ifdef AALLOC_TRACK
	TCooked prev;
#ifndef AALLOC_NO_COOKIES
	TCookie scookie;
#endif
#ifdef AALLOC_STATS
	const char *name;
	unsigned long numalloc;
	unsigned long maxalloc;
	bool isfree;
#endif
#endif
};

static TCookie fcookie;

#ifdef AALLOC_TRACK
static PArea track;
static void track_check(void);
#endif

#ifdef AALLOC_MPROTECT
#undef AALLOC_INITSZ
#define AALLOC_INITSZ		pagesz
static long pagesz;
#define AALLOC_ALLOW(bp)	mprotect((bp), (bp)->endp - (char *)(bp), \
				    PROT_READ | PROT_WRITE)
#define AALLOC_DENY(bp)		mprotect((bp), (bp)->endp - (char *)(bp), \
				    PROT_NONE)
#define AALLOC_PEEK(bp)		mprotect((bp), sizeof (struct TArea), \
				    PROT_READ | PROT_WRITE)
#else
#define AALLOC_ALLOW(bp)	/* nothing */
#define AALLOC_DENY(bp)		/* nothing */
#define AALLOC_PEEK(bp)		/* nothing */
#endif

/*
 * Some nice properties: allocations are always PVALIGNed, which
 * includes the pointers seen by our user, the forward and back
 * pointers, the AALLOC_TRACK prev pointer, etc.
 */

#define safe_malloc(dest, len) do {					\
	(dest) = malloc((len));						\
	safe_xalloc_common((dest), (len));				\
} while (/* CONSTCOND */ 0)
#define safe_realloc(dest, len) do {					\
	(dest) = realloc((dest), (len));				\
	safe_xalloc_common((dest), (len));				\
} while (/* CONSTCOND */ 0)
#define safe_xalloc_common(dest, len) do {				\
	if ((dest) == NULL)						\
		AALLOC_ABORT("unable to allocate %lu bytes: %s",	\
		    (unsigned long)(len), strerror(errno));		\
	if (isunaligned(dest))						\
		AALLOC_ABORT("unaligned malloc result: %p", (dest));	\
} while (/* CONSTCOND */ 0)

#define MUL_NO_OVERFLOW (1UL << (sizeof (size_t) * 8 / 2))
#define safe_muladd(nmemb, size, extra) do {				\
	if ((nmemb >= MUL_NO_OVERFLOW || size >= MUL_NO_OVERFLOW) &&	\
	    nmemb > 0 && SIZE_MAX / nmemb < size)			\
		AALLOC_ABORT("attempted integer overflow: %lu * %lu",	\
		    (unsigned long)nmemb, (unsigned long)size);		\
	size *= nmemb;							\
	if (size >= SIZE_MAX - extra)					\
		AALLOC_ABORT("unable to allocate %lu bytes: %s",	\
		    (unsigned long)size, "value plus extra too big");	\
	size += extra;							\
} while (/* CONSTCOND */ 0)

static void adelete_leak(PArea, PBlock, bool, const char *);
static PBlock check_bp(PArea, const char *, TCookie);
static TCooked *check_ptr(void *, PArea, PBlock *, TPtr *, const char *,
    const char *);

PArea
#ifdef AALLOC_STATS
anewEx(size_t hint, const char *friendly_name)
#else
anew(size_t hint)
#endif
{
	PArea ap;
	PBlock bp;
	TPtr xp;

	AALLOC_THREAD_ENTER(NULL)

#ifdef AALLOC_MPROTECT
	if (!pagesz) {
		if ((pagesz = sysconf(_SC_PAGESIZE)) == -1 ||
		    (size_t)pagesz < (size_t)PVALIGN)
			AALLOC_ABORT("sysconf(_SC_PAGESIZE) failed: %ld %s",
			    pagesz, strerror(errno));
	}
#endif

#ifdef AALLOC_DEBUG
	if (PVALIGN != 2 && PVALIGN != 4 && PVALIGN != 8 && PVALIGN != 16)
		AALLOC_ABORT("PVALIGN not a power of two: %lu",
		    (unsigned long)PVALIGN);
	if (sizeof (TPtr) != sizeof (TCookie) || sizeof (TPtr) != PVALIGN ||
	    sizeof (TPtr) != sizeof (TCooked))
		AALLOC_ABORT("TPtr sizes do not match: %lu, %lu, %lu, %lu",
		    (unsigned long)sizeof (TPtr),
		    (unsigned long)sizeof (TCookie), (unsigned long)PVALIGN,
		    (unsigned long)sizeof (TCooked));
	if ((size_t)AALLOC_INITSZ < sizeof (struct TBlock))
		AALLOC_ABORT("AALLOC_INITSZ constant too small: %lu < %lu",
		    (unsigned long)AALLOC_INITSZ,
		    (unsigned long)sizeof (struct TBlock));
	if (hint != (1UL << (ffs(hint) - 1)))
		AALLOC_ABORT("anew hint %lu not a power of two or too big",
		    (unsigned long)hint);
#endif

	if (!fcookie) {
#ifdef AALLOC_NO_COOKIES
		fcookie++;
#else
		size_t v = 0;

		/* ensure unaligned cookie */
		do {
			fcookie = AALLOC_RANDOM();
			if (AALLOC_RANDOM() & 1)
				v = AALLOC_RANDOM() & 7;
		} while (!iscook(fcookie) || !v);
		/* randomise seed afterwards */
		while (v--)
			AALLOC_RANDOM();
#endif
#ifdef AALLOC_TRACK
		atexit(track_check);
#endif
	}

	safe_muladd(sizeof (void *), hint, 0);
	if (hint < sizeof (struct TBlock))
		hint = AALLOC_INITSZ;

	safe_malloc(ap, sizeof (struct TArea));
	safe_malloc(bp, hint);
	/* ensure unaligned cookie */
#ifndef AALLOC_NO_COOKIES
	do {
		bp->bcookie = AALLOC_RANDOM();
	} while (!iscook(bp->bcookie));
#endif

	/* first byte after block */
	bp->endp = (char *)bp + hint;		/* bp + size of the block */
	/* next entry (forward pointer) available for new allocation */
	bp->last = (char *)&bp->storage;	/* first entry */

	(void)stcookp(ap->bp, bp, fcookie);
#ifdef AALLOC_TRACK
	(void)stcookp(ap->prev, track, fcookie);
#ifndef AALLOC_NO_COOKIES
	(void)stcooki(ap->scookie, bp->bcookie, fcookie);
#endif
#ifdef AALLOC_STATS
	ap->name = friendly_name ? friendly_name : "(no name)";
	ap->numalloc = 0;
	ap->maxalloc = 0;
	ap->isfree = false;
#endif
	track = ap;
#endif
	AALLOC_DENY(bp);
	AALLOC_THREAD_LEAVE(NULL)
	return (ap);
}

/*
 * Validate block in Area “ap”, return unlocked block pointer.
 * If “ocookie” is not 0, make sure block cookie is equal.
 */
static PBlock
check_bp(PArea ap, const char *funcname, TCookie ocookie __unused)
{
	TPtr p;
	PBlock bp;

	(void)ldcook(p, ap->bp, fcookie);
#ifndef AALLOC_SMALL
	if (!p.nv
#ifdef AALLOC_STATS
	    || ap->isfree
#endif
	    ) {
		AALLOC_WARN("%s: area %p already freed", funcname, ap);
		return (NULL);
	}
	if (isunaligned(p.nv)) {
		AALLOC_WARN("%s: area %p block pointer destroyed: %p",
		    funcname, ap, p.cp);
		return (NULL);
	}
#endif
	bp = (PBlock)p.cp;
	AALLOC_PEEK(bp);
#ifndef AALLOC_NO_COOKIES
	if (ocookie && bp->cookie != ocookie) {
		AALLOC_WARN("%s: block %p cookie destroyed: %p, %p",
		    funcname, bp, (void *)ocookie, (void *)bp->cookie);
		return (NULL);
	}
#endif
#ifndef AALLOC_SMALL
	if (isunaligned(bp->endp) || isunaligned(bp->last)) {
		AALLOC_WARN("%s: block %p data structure destroyed: %p, %p",
		    funcname, bp, bp->endp, bp->last);
		return (NULL);
	}
	if (bp->endp < (char *)bp) {
		AALLOC_WARN("%s: block %p end pointer out of bounds: %p",
		    funcname, bp, bp->endp);
		return (NULL);
	}
	if ((bp->last < (char *)&bp->storage) || (bp->last > bp->endp)) {
		AALLOC_WARN("%s: block %p last pointer out of bounds: "
		    "%p < %p < %p", funcname, bp, &bp->storage, bp->last,
		    bp->endp);
		return (NULL);
	}
#endif
	AALLOC_ALLOW(bp);
	return (bp);
}

#ifdef AALLOC_TRACK
/*
 * At exit, dump and free any leaked allocations, blocks and areas.
 */
static void
track_check(void)
{
	PArea tp;
	PBlock bp;
	TCookie xc = 0;
	TPtr xp;

	AALLOC_THREAD_ENTER(NULL)
	tp = track;
	while (tp) {
#ifndef AALLOC_NO_COOKIES
		xc = ldcook(xp, tp->scookie, fcookie);
#endif
		(void)ldcook(xp, tp->prev, fcookie);
#ifdef AALLOC_STATS
		AALLOC_WARN("AALLOC_STATS for %s(%p): %lu allocated, %lu at "
		    "once, %sfree", tp->name, tp, tp->numalloc, tp->maxalloc,
		    tp->isfree ? "" : "not ");
		if (tp->isfree)
			goto track_check_next;
#endif
		if (isunaligned(xp.nv) || !iscook(xc)) {
			/* buffer overflow or something? */
			AALLOC_WARN("AALLOC_TRACK data structure %p destroyed:"
			    " %p, %p; exiting", tp, xp.cp, (void *)xc);
			break;
		}
		if (!(bp = check_bp(tp, "atexit:track_check", xc)))
			goto track_check_next;
		if (bp->last != (char *)&bp->storage)
#ifdef AALLOC_LEAK_SILENT
			adelete_leak(tp, bp, false, "at exit");
#else
			adelete_leak(tp, bp, true, "at exit");
		else
			AALLOC_WARN("leaking empty area %p (%p %lu)", tp,
			    bp, (unsigned long)(bp->endp - (char *)bp));
#endif
		free(bp);
 track_check_next:
		free(tp);
		tp = (PArea)xp.cp;
	}
	track = NULL;
	AALLOC_THREAD_LEAVE(NULL)
}
#endif

static void
adelete_leak(PArea ap, PBlock bp, bool always_warn, const char *when)
{
	TPtr xp;

	while (bp->last > (char *)&bp->storage) {
		bp->last -= PVALIGN;
		(void)ldcook(xp, **((TCooked **)bp->last), bp->bcookie);
#ifndef AALLOC_SMALL
		if (always_warn || xp.cp != bp->last)
			AALLOC_WARN("leaking %s pointer %p in area %p (ofs %p "
			    "len %lu) %s", xp.cp != bp->last ? "underflown" :
			    "valid", *((char **)bp->last) + PVALIGN, ap, bp,
			    (unsigned long)(bp->endp - (char *)bp), when);
#endif
		free(xp.cp);
	}
}

void
adelete(PArea *pap)
{
	PBlock bp;
#if defined(AALLOC_TRACK) && !defined(AALLOC_STATS)
	PArea tp;
	TCookie xc = 0;
	TPtr xp;
#endif

	AALLOC_THREAD_ENTER(*pap)

	/* ignore invalid areas */
	if ((bp = check_bp(*pap, "adelete", 0)) != NULL) {
		if (bp->last != (char *)&bp->storage)
			adelete_leak(*pap, bp, false, "in adelete");
		free(bp);
	}

#if defined(AALLOC_TRACK) && !defined(AALLOC_STATS)
	/* if we are the last TArea allocated */
	if (track == *pap) {
		if (isunaligned(ldcook(xp, (*pap)->prev, fcookie))) {
			AALLOC_WARN("AALLOC_TRACK data structure %p destroyed:"
			    " %p", *pap, xp.cp);
			track = NULL;
		} else
			track = (PArea)xp.cp;
		goto adelete_tracked;
	}
	/* find the TArea whose prev is *pap */
	tp = track;
	while (tp) {
#ifndef AALLOC_NO_COOKIES
		xc = ldcook(xp, tp->scookie, fcookie);
#endif
		(void)ldcook(xp, tp->prev, fcookie);
		if (isunaligned(xp.nv) || !iscook(xc)) {
			/* buffer overflow or something? */
			AALLOC_WARN("AALLOC_TRACK data structure %p destroyed:"
			    " %p, %p; ignoring", tp, xp.cp, (void *)xc);
			tp = NULL;
			break;
		}
		if (xp.cp == (char *)*pap)
			break;
		tp = (PArea)xp.cp;
	}
	if (tp)
		tp->prev.xv = (*pap)->prev.xv;	/* decouple *pap */
	else
		AALLOC_WARN("area %p not in found AALLOC_TRACK data structure",
		    *pap);
 adelete_tracked:
#endif
	AALLOC_THREAD_LEAVE(*pap)
#ifdef AALLOC_STATS
	(*pap)->isfree = true;
#else
	free(*pap);
#endif
	*pap = NULL;
}

void *
alloc(size_t nmemb, size_t size, PArea ap)
{
	PBlock bp;
	TCooked *rp;
	TPtr xp;

	/* obtain the memory region requested, retaining guards */
	safe_muladd(nmemb, size, sizeof (TPtr));
	safe_malloc(rp, size);

	AALLOC_THREAD_ENTER(ap)

	/* chain into area */
	if ((bp = check_bp(ap, "alloc", 0)) == NULL)
		AALLOC_ABORT("cannot continue");
	if (bp->last == bp->endp) {
		TCooked **pp;
		size_t bsz;

		/* make room for more forward ptrs in the block allocation */
		bsz = bp->endp - (char *)bp;
		safe_muladd(2, bsz, 0);
		safe_realloc(bp, bsz);
		bp->last = (char *)bp + (bsz / 2);
		bp->endp = (char *)bp + bsz;

		/* all backpointers have to be adjusted */
		pp = (TCooked **)&bp->storage;
		while (pp < (TCooked **)bp->last) {
			(void)stcookp(**pp, pp, bp->bcookie);
			++pp;
		}

		/* “bp” has possibly changed, enter its new value into ap */
		(void)stcookp(ap->bp, bp, fcookie);
	}
	memcpy(bp->last, &rp, PVALIGN);	/* next available forward ptr */
	/* store cooked backpointer to address of forward pointer */
	(void)stcookp(*rp, bp->last, bp->bcookie);
	bp->last += PVALIGN;		/* advance next-avail pointer */
#ifdef AALLOC_STATS
	ap->numalloc++;
	{
		unsigned long curalloc;

		curalloc = (bp->last - (char *)&bp->storage) / PVALIGN;
		if (curalloc > ap->maxalloc)
			ap->maxalloc = curalloc;
	}
#endif
	AALLOC_DENY(bp);
	AALLOC_THREAD_LEAVE(ap)

	/* return aligned storage just after the cookied backpointer */
	return ((char *)rp + PVALIGN);
}

void *
aresize(void *vp, size_t nmemb, size_t size, PArea ap)
{
	PBlock bp;
	TCooked *rp;
	TPtr xp;

	if (vp == NULL)
		return (alloc(nmemb, size, ap));

	AALLOC_THREAD_ENTER(ap)

	/* validate allocation and backpointer against forward pointer */
	if ((rp = check_ptr(vp, ap, &bp, &xp, "aresize", "")) == NULL)
		AALLOC_ABORT("cannot continue");

	/* move allocation to size and possibly new location */
	safe_muladd(nmemb, size, sizeof (TPtr));
	safe_realloc(rp, size);

	/* write new address of allocation into the block forward pointer */
	memcpy(xp.cp, &rp, PVALIGN);

	AALLOC_DENY(bp);
	AALLOC_THREAD_LEAVE(ap)

	return ((char *)rp + PVALIGN);
}

/*
 * Try to find “vp” inside Area “ap”, use “what” and “extra” for error msgs.
 *
 * If an error occurs, returns NULL with no side effects.
 * Otherwise, returns address of the allocation, *bpp contains the unlocked
 * block pointer, *xpp the uncookied backpointer.
 */
static TCooked *
check_ptr(void *vp, PArea ap, PBlock *bpp, TPtr *xpp, const char *what,
    const char *extra)
{
	TCooked *rp;

#ifndef AALLOC_SMALL
	if (isunaligned(vp)) {
		AALLOC_WARN("trying to %s rogue unaligned pointer %p from "
		    "area %p%s", what + 1, vp, ap, extra);
		return (NULL);
	}
#endif

	rp = (TCooked *)(((char *)vp) - PVALIGN);
#ifndef AALLOC_SMALL
	if (!rp->xv) {
		AALLOC_WARN("trying to %s already freed pointer %p from "
		    "area %p%s", what + 1, vp, ap, extra);
		return (NULL);
	}
#endif

	if ((*bpp = check_bp(ap, what, 0)) == NULL)
		AALLOC_ABORT("cannot continue");
#ifndef AALLOC_SMALL
	if (isunaligned(ldcook(*xpp, *rp, (*bpp)->bcookie))) {
		AALLOC_WARN("trying to %s rogue pointer %p from area %p "
		    "(block %p..%p), backpointer %p unaligned%s",
		    what + 1, vp, ap, *bpp, (*bpp)->last, xpp->cp, extra);
	} else if (xpp->cp < (char *)&(*bpp)->storage ||
	    xpp->cp >= (*bpp)->last) {
		AALLOC_WARN("trying to %s rogue pointer %p from area %p "
		    "(block %p..%p), backpointer %p out of bounds%s",
		    what + 1, vp, ap, *bpp, (*bpp)->last, xpp->cp, extra);
	} else if (*((TCooked **)xpp->cp) != rp) {
		AALLOC_WARN("trying to %s rogue pointer %p from area %p "
		    "(block %p..%p), backpointer %p, forward pointer to "
		    "%p instead%s", what + 1, vp, ap, *bpp, (*bpp)->last,
		    xpp->cp, *((TCooked **)xpp->cp), extra);
	} else
#endif
		return (rp);

#ifndef AALLOC_SMALL
	/* error case fall-through */
	AALLOC_DENY(*bpp);
	return (NULL);
#endif
}

void
afree(void *vp, PArea ap)
{
	PBlock bp;
	TCooked *rp;
	TPtr xp;

	if (vp == NULL)
		return;

	AALLOC_THREAD_ENTER(ap)

	/* validate allocation and backpointer, ignore rogues */
	if ((rp = check_ptr(vp, ap, &bp, &xp, "afree", ", ignoring")) == NULL)
		goto afree_done;

	/* note: the block allocation does not ever shrink */
	bp->last -= PVALIGN;	/* mark the last forward pointer as free */
	/* if our forward pointer was not the last one, relocate the latter */
	if (xp.cp < bp->last) {
		TCooked *tmp = *((TCooked **)bp->last);

		(void)stcook(*tmp, xp, bp->bcookie);	/* write new backptr */
		memcpy(xp.cp, bp->last, PVALIGN);	/* relocate fwd ptr */
	}
	rp->xv = 0;	/* our backpointer, just in case, for double frees */
	free(rp);

	AALLOC_DENY(bp);
 afree_done:
	AALLOC_THREAD_LEAVE(ap)
	return;
}
