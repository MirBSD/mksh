#include "sh.h"

__RCSID("$MirOS: src/bin/mksh/aalloc.c,v 1.26 2008/11/15 07:35:23 tg Exp $");

/* mksh integration of aalloc */

#ifndef AALLOC_ABORT
#define AALLOC_ABORT		internal_errorf
#endif

#ifndef AALLOC_WARN
#define AALLOC_WARN		internal_warningf
#endif

#define AALLOC_LEAK_SILENT	/* the code does not yet clean up at exit */

#ifndef AALLOC_RANDOM
#if HAVE_ARC4RANDOM
#define AALLOC_RANDOM()		arc4random()
#else
#define AALLOC_RANDOM()		(rand() * RAND_MAX + rand())
#endif
#endif

#ifdef DEBUG
#define AALLOC_DEBUG
#endif

/* generic area-based allocator built for mmap malloc or omalloc */

#define PVALIGN			(sizeof (void *))
#define PVMASK			(sizeof (void *) - 1)

#ifndef AALLOC_INITSZ
#define AALLOC_INITSZ		(64 * PVALIGN)	/* at least 4 pointers */
#endif

typedef /* unsigned */ ptrdiff_t TCookie;

typedef union {
	TCookie iv;
	char *pv;
} TPtr;

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
	TCookie cookie;
	char *endp;
	char *last;
	void *storage;
};
typedef struct TBlock *PBlock;

struct TArea {
	TPtr bp;
#ifdef AALLOC_TRACK
	TPtr prev;
	TCookie ocookie;
#endif
};

static TCookie global_cookie;
#ifdef AALLOC_NO_COOKIES
#define gcookie 0
#else
#define gcookie global_cookie
#endif

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
	if ((ptrdiff_t)(dest) & PVMASK)					\
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
static TPtr *check_ptr(void *, PArea, PBlock *, const char *, const char *);

PArea
anew(size_t hint)
{
	PArea ap;
	PBlock bp;

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
	if (sizeof (TPtr) != sizeof (TCookie) || sizeof (TPtr) != PVALIGN)
		AALLOC_ABORT("TPtr sizes do not match: %lu, %lu, %lu",
		    (unsigned long)sizeof (TPtr),
		    (unsigned long)sizeof (TCookie), (unsigned long)PVALIGN);
	if ((size_t)AALLOC_INITSZ < sizeof (struct TBlock))
		AALLOC_ABORT("AALLOC_INITSZ constant too small: %lu < %lu",
		    (unsigned long)AALLOC_INITSZ,
		    (unsigned long)sizeof (struct TBlock));
	if (hint != (1UL << (ffs(hint) - 1)))
		AALLOC_ABORT("anew hint %lu not a power of two or too big",
		    (unsigned long)hint);
#endif

	if (!global_cookie) {
		size_t v;

		/* ensure unaligned cookie */
		do {
			global_cookie = AALLOC_RANDOM();
			v = AALLOC_RANDOM() & 7;
		} while (!(global_cookie & PVMASK) || !v);
		/* randomise seed afterwards */
		while (v--)
			AALLOC_RANDOM();
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
#ifdef AALLOC_NO_COOKIES
	bp->cookie = 0;
#else
	do {
		bp->cookie = AALLOC_RANDOM();
	} while (!(bp->cookie & PVMASK));
#endif

	/* first byte after block */
	bp->endp = (char *)bp + AALLOC_INITSZ;	/* bp + size of the block */
	/* next entry (forward pointer) available for new allocation */
	bp->last = (char *)&bp->storage;	/* first entry */

	ap->bp.pv = (char *)bp;
	ap->bp.iv ^= gcookie;
#ifdef AALLOC_TRACK
	ap->prev.pv = (char *)track;
	ap->prev.iv ^= gcookie;
	ap->ocookie = bp->cookie ^ gcookie;
	track = ap;
#endif
	AALLOC_DENY(bp);
	return (ap);
}

/*
 * Validate block in Area “ap”, return unlocked block pointer.
 * If “ocookie” is not 0, make sure block cookie is equal.
 */
static PBlock
check_bp(PArea ap, const char *funcname, TCookie ocookie)
{
	TPtr p;
	PBlock bp;

	if (ap->bp.pv == NULL) {
		AALLOC_WARN("%s: area %p already freed", funcname, ap);
		return (NULL);
	}
	p.iv = ap->bp.iv ^ gcookie;
	if ((ptrdiff_t)(bp = (PBlock)p.pv) & PVMASK) {
		AALLOC_WARN("%s: area %p block pointer destroyed: %p",
		    funcname, ap, p.pv);
		return (NULL);
	}
	AALLOC_PEEK(bp);
	if (ocookie && bp->cookie != ocookie) {
		AALLOC_WARN("%s: block %p cookie destroyed: %p, %p",
		    funcname, bp, (void *)ocookie, (void *)bp->cookie);
		return (NULL);
	}
	if (((ptrdiff_t)bp->endp & PVMASK) || ((ptrdiff_t)bp->last & PVMASK)) {
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
	TPtr lp;
	PBlock bp;

	while (track) {
		tp = track;
		tp->ocookie ^= gcookie;
		lp.iv = tp->prev.iv ^ gcookie;
		if ((lp.iv & PVMASK)
#ifndef AALLOC_NO_COOKIES
		    || !(tp->ocookie & PVMASK)
#endif
		    ) {
			/* buffer overflow or something? */
			AALLOC_WARN("AALLOC_TRACK data structure %p destroyed:"
			    " %p, %p, %p; exiting", tp, lp.pv, tp->bp.pv,
			    (void *)tp->ocookie);
			return;
		}
		if (!(bp = check_bp(tp, "atexit:track_check", tp->ocookie)))
			goto track_next;
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
 track_next:
		track = (PArea)lp.pv;
		free(tp);
	}
}
#endif

static void
adelete_leak(PArea ap, PBlock bp, bool always_warn, const char *when)
{
	TPtr *cp;

	while (bp->last > (char *)&bp->storage) {
		bp->last -= PVALIGN;
		cp = *((void **)bp->last);
		cp->iv ^= bp->cookie;
		if (always_warn || cp->pv != bp->last)
		AALLOC_WARN("leaking %s pointer %p in area %p (%p %lu) %s",
		    cp->pv == bp->last ? "valid" : "underflown",
		    (char *)cp + PVALIGN, ap, bp,
		    (unsigned long)(bp->endp - (char *)bp), when);
		free(cp);
	}
}

void
adelete(PArea *pap)
{
#ifdef AALLOC_TRACK
	PArea tp;
	TPtr lp;
#endif
	PBlock bp;

	/* ignore invalid areas */
	if ((bp = check_bp(*pap, "adelete", 0)) != NULL) {
		if (bp->last != (char *)&bp->storage)
			adelete_leak(*pap, bp, false, "in adelete");
		free(bp);
	}

#ifdef AALLOC_TRACK
	if (track == *pap) {
		(*pap)->prev.iv ^= gcookie;
		track = (PArea)((*pap)->prev.pv);
		goto adelete_tracked;
	}
	/* find the TArea whose prev is *pap */
	tp = track;
	while (tp) {
		lp.iv = tp->prev.iv ^ gcookie;
		if ((lp.iv & PVMASK)
#ifndef AALLOC_NO_COOKIES
		    || !((tp->ocookie ^ gcookie) & PVMASK)
#endif
		    ) {
			/* buffer overflow or something? */
			AALLOC_WARN("AALLOC_TRACK data structure %p destroyed:"
			    " %p, %p, %p; exiting", tp, lp.pv, tp->bp.pv,
			    (void *)(tp->ocookie ^ gcookie));
			tp = NULL;
			break;
		}
		if (lp.pv == (char *)*pap)
			break;
		tp = (PArea)lp.pv;
	}
	if (tp)
		tp->prev.iv = (*pap)->prev.iv;	/* decouple *pap */
	else
		AALLOC_WARN("area %p not in found AALLOC_TRACK data structure",
		    *pap);
 adelete_tracked:
#endif
	free(*pap);
	*pap = NULL;
}

void *
alloc(size_t nmemb, size_t size, PArea ap)
{
	PBlock bp;
	TPtr *ptr;

	/* obtain the memory region requested, retaining guards */
	safe_muladd(nmemb, size, sizeof (TPtr));
	safe_malloc(ptr, size);

	/* chain into area */
	if ((bp = check_bp(ap, "alloc", 0)) == NULL)
		AALLOC_ABORT("cannot continue");
	if (bp->last == bp->endp) {
		TPtr **tp;
		size_t bsz;

		/* make room for more forward ptrs in the block allocation */
		bsz = bp->endp - (char *)bp;
		safe_muladd(2, bsz, 0);
		safe_realloc(bp, bsz);
		bp->last = (char *)bp + (bsz / 2);
		bp->endp = (char *)bp + bsz;

		/* all backpointers have to be adjusted */
		for (tp = (TPtr **)&bp->storage; tp < (TPtr **)bp->last; ++tp) {
			(*tp)->pv = (char *)tp;
			(*tp)->iv ^= bp->cookie;
		}

		/* “bp” has possibly changed, enter its new value into ap */
		ap->bp.pv = (char *)bp;
		ap->bp.iv ^= gcookie;
	}
	*((void **)bp->last) = ptr;	/* next available forward ptr */
	ptr->pv = bp->last;		/* backpointer to fwdptr storage */
	ptr->iv ^= bp->cookie;		/* apply block cookie */
	bp->last += PVALIGN;		/* advance next-avail pointer */
	AALLOC_DENY(bp);

	/* return aligned storage just after the cookied backpointer */
	return ((char *)ptr + PVALIGN);
}

void *
aresize(void *vp, size_t nmemb, size_t size, PArea ap)
{
	PBlock bp;
	TPtr *ptr;

	if (vp == NULL)
		return (alloc(nmemb, size, ap));

	/* validate allocation and backpointer against forward pointer */
	if ((ptr = check_ptr(vp, ap, &bp, "aresize", "")) == NULL)
		AALLOC_ABORT("cannot continue");

	/* move allocation to size and possibly new location */
	safe_muladd(nmemb, size, sizeof (TPtr));
	safe_realloc(ptr, size);

	/* write new address of allocation into the block forward pointer */
	memcpy(ptr->pv, &ptr, PVALIGN);
	/* apply the cookie on the backpointer again */
	ptr->iv ^= bp->cookie;
	AALLOC_DENY(bp);

	return ((char *)ptr + PVALIGN);
}

/*
 * Try to find “vp” inside Area “ap”, use “what” and “extra” for error msgs.
 *
 * If an error occurs, returns NULL with no side effects.
 * Otherwise, returns address of the allocation, with the cookie on the
 * backpointer unapplied; *bpp contains the unlocked block pointer.
 */
static TPtr *
check_ptr(void *vp, PArea ap, PBlock *bpp, const char *what, const char *extra)
{
	PBlock bp;
	TPtr *ptr;

	if ((ptrdiff_t)vp & PVMASK) {
		AALLOC_WARN("trying to %s rogue unaligned pointer %p from "
		    "area %p%s", what + 1, vp, ap, extra);
		return (NULL);
	}

	ptr = (TPtr *)((char *)vp - PVALIGN);
	if (!ptr->iv) {
		AALLOC_WARN("trying to %s already freed pointer %p from "
		    "area %p%s", what + 1, vp, ap, extra);
		return (NULL);
	}

	if ((bp = check_bp(ap, what, 0)) == NULL)
		AALLOC_ABORT("cannot continue");
	ptr->iv ^= bp->cookie;
	if (ptr->pv < (char *)&bp->storage || ptr->pv >= bp->last) {
		AALLOC_WARN("trying to %s rogue pointer %p from area %p "
		    "(block %p..%p), backpointer %p out of bounds%s",
		    what + 1, vp, ap, bp, bp->last, ptr->pv, extra);
		AALLOC_DENY(bp);
		return (NULL);
	}
	if (*((void **)ptr->pv) != ptr) {
		AALLOC_WARN("trying to %s rogue pointer %p from area %p "
		    "(block %p..%p), backpointer %p, forward pointer to "
		    "%p instead%s", what + 1, vp, ap, bp, bp->last,
		    ptr->pv, *((void **)ptr->pv), extra);
		AALLOC_DENY(bp);
		return (NULL);
	}
	*bpp = bp;
	return (ptr);
}

void
afree(void *vp, PArea ap)
{
	PBlock bp;
	TPtr *ptr;

	if (vp == NULL)
		return;

	/* validate allocation and backpointer, ignore rogues */
	if ((ptr = check_ptr(vp, ap, &bp, "afree", ", ignoring")) == NULL)
		return;

	/* note: the block allocation does not ever shrink */
	bp->last -= PVALIGN;	/* mark the last forward pointer as free */
	/* if our forward pointer was not the last one, relocate the latter */
	if (ptr->pv < bp->last) {
		TPtr *tmp = *((TPtr **)bp->last);

		/* tmp is the former last forward pointer */
		tmp->pv = ptr->pv;	/* its backpointer to former our … */
		tmp->iv ^= bp->cookie;	/* … forward pointer, and cookie it */

		memcpy(ptr->pv, bp->last, PVALIGN);	/* relocate fwd ptr */
	}
	ptr->iv = 0;	/* our backpointer, just in case, for double frees */
	free(ptr);

	AALLOC_DENY(bp);
	return;
}
