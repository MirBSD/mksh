/**	$MirBSD: src/bin/ksh/tty.h,v 2.2 2004/12/31 17:39:12 tg Exp $ */
/*	$OpenBSD: tty.h,v 1.4 2004/12/18 22:12:23 millert Exp $	*/

#ifndef TTY_H
#define TTY_H

/* some useful #defines */
#ifdef EXTERN
# define I__(i) = i
#else
# define I__(i)
# define EXTERN extern
# define EXTERN_DEFINED
#endif

/* Don't know of a system on which including sys/ioctl.h with termios.h
 * causes problems.  If there is one, these lines need to be deleted and
 * aclocal.m4 needs to have stuff un-commented.
 */
#ifdef SYS_IOCTL_WITH_TERMIOS
# define SYS_IOCTL_WITH_TERMIOS
#endif /* SYS_IOCTL_WITH_TERMIOS */
#ifdef SYS_IOCTL_WITH_TERMIO
# define SYS_IOCTL_WITH_TERMIO
#endif /* SYS_IOCTL_WITH_TERMIO */

#include <termios.h>
typedef struct termios TTY_state;

EXTERN int		tty_fd I__(-1);	/* dup'd tty file descriptor */
EXTERN int		tty_devtty;	/* true if tty_fd is from /dev/tty */
EXTERN struct termios	tty_state;	/* saved tty state */

extern void	tty_init(int init_ttystate);
extern void	tty_close(void);

/* be sure not to interfere with anyone else's idea about EXTERN */
#ifdef EXTERN_DEFINED
# undef EXTERN_DEFINED
# undef EXTERN
#endif
#undef I__

#endif	/* ndef TTY_H */
