/**	$MirBSD: src/bin/ksh/ksh_limval.h,v 1.1.15.1 2004/12/10 18:08:08 tg Exp $ */
/*	$OpenBSD: ksh_limval.h,v 1.1.1.1 1996/08/14 06:19:11 downsj Exp $	*/

#ifndef KSH_LIMVAL_H
#define KSH_LIMVAL_H

/* Wrapper around the values.h/limits.h includes/ifdefs */

#ifdef HAVE_VALUES_H
# include <values.h>
#endif /* HAVE_VALUES_H */
/* limits.h is included in sh.h */

#ifndef DMAXEXP
# define DMAXEXP	128	/* should be big enough */
#endif

#ifndef BITSPERBYTE
# ifdef CHAR_BIT
#  define BITSPERBYTE	CHAR_BIT
# else
#  define BITSPERBYTE	8	/* probably true.. */
# endif
#endif

#ifndef BITS
# define BITS(t)	(BITSPERBYTE * sizeof(t))
#endif

#endif	/* ndef KSH_LIMVAL_H */
