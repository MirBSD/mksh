/**	$MirBSD: conf-end.h,v 1.9 2004/10/28 16:35:01 tg Exp $ */
/*	$OpenBSD: conf-end.h,v 1.2 1996/08/25 12:37:58 downsj Exp $	*/

#ifndef CONF_END_H
#define CONF_END_H

/* Include ksh features? */
/* #define KSH 1 */

/* Include emacs editing? */
#define EMACS 1

/* Include vi editing? */
#define VI 1

/* Include job control? */
#define JOBS 1

/* Include brace-expansion? */
#define BRACE_EXPAND 1

/* Include any history? */
#define HISTORY 1

/* Include complex history? */
#define COMPLEX_HISTORY

/* Strict POSIX behaviour? */
#ifdef KSH
#undef POSIXLY_CORRECT
#else
#define POSIXLY_CORRECT 1
#endif

/* Specify default $ENV? */
/* #undef DEFAULT_ENV */

/*
 * The above are defined for mirbsdksh via external
 * means, such as this header ;-)
 * End of configuration stuff for PD ksh.
 */

#if defined(EMACS) || defined(VI)
# define	EDIT
#else
# undef		EDIT
#endif

/* Super small configuration-- no editing. */
#if defined(EDIT) && defined(NOEDIT)
# undef EDIT
# undef EMACS
# undef VI
#endif

/* Editing implies history */
#if defined(EDIT) && !defined(HISTORY)
# define HISTORY
#endif /* EDIT */

/*
 * if you don't have mmap() you can't use Peter Collinson's history
 * mechanism.  If that is the case, then define EASY_HISTORY
 */
#if defined(HISTORY) && (!defined(COMPLEX_HISTORY) || !defined(HAVE_MMAP) || !defined(HAVE_FLOCK))
# undef COMPLEX_HISTORY
# define EASY_HISTORY			/* sjg's trivial history file */
#endif

/* Can we safely catch sigchld and wait for processes? */
#if (defined(HAVE_WAITPID) || defined(HAVE_WAIT3)) \
    && (defined(POSIX_SIGNALS) || defined(BSD42_SIGNALS))
# define JOB_SIGS
#endif

#if !defined(JOB_SIGS) || !(defined(POSIX_PGRP) || defined(BSD_PGRP))
# undef JOBS /* if no JOB_SIGS, no job control support */
#endif

#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#else
# ifdef HAVE_SYS_TYPES_H
#  include <sys/types.h>
# endif /* HAVE_SYS_TYPES_H */
#endif /* HAVE_SYS_PARAM_H */

#ifdef HAVE_STDINT_H
# include <stdint.h>
#endif

/* pdksh assumes system calls return EINTR if a signal happened (this so
 * the signal handler doesn't have to longjmp()).  I don't know if this
 * happens (or can be made to happen) with sigset() et. al. (the bsd41 signal
 * routines), so, the autoconf stuff checks what they do and defines
 * SIGNALS_DONT_INTERRUPT if signals don't interrupt read().
 * If SIGNALS_DONT_INTERRUPT isn't defined and your compiler chokes on this,
 * delete the hash in front of the error (and file a bug report).
 */
#ifdef SIGNALS_DONT_INTERRUPT
  # error pdksh needs interruptable system calls.
#endif /* SIGNALS_DONT_INTERRUPT */

#ifdef HAVE_GCC_FUNC_ATTR
# define GCC_FUNC_ATTR(x)	__attribute__((x))
# define GCC_FUNC_ATTR2(x,y)	__attribute__((x,y))
#else
# define GCC_FUNC_ATTR(x)
# define GCC_FUNC_ATTR2(x,y)
#endif /* HAVE_GCC_FUNC_ATTR */

#endif	/* ndef CONF_END_H */
