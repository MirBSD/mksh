/**	$MirBSD: src/bin/ksh/tty.c,v 2.3 2004/12/31 17:39:12 tg Exp $ */
/*	$OpenBSD: tty.c,v 1.4 2004/12/18 22:12:23 millert Exp $	*/

#include "sh.h"
#include "ksh_stat.h"
#define EXTERN
#include "tty.h"
#undef EXTERN

__RCSID("$MirBSD: src/bin/ksh/tty.c,v 2.3 2004/12/31 17:39:12 tg Exp $");

/* Initialize tty_fd.  Used for saving/reseting tty modes upon
 * foreground job completion and for setting up tty process group.
 */
void
tty_init(int init_ttystate)
{
	int	do_close = 1;
	int	tfd;

	if (tty_fd >= 0) {
		close(tty_fd);
		tty_fd = -1;
	}
	tty_devtty = 1;

	if ((tfd = open("/dev/tty", O_RDWR, 0)) < 0) {
			tty_devtty = 0;
			warningf(false,
				"No controlling tty (open /dev/tty: %s)",
				strerror(errno));
		do_close = 0;
		if (isatty(0))
			tfd = 0;
		else if (isatty(2))
			tfd = 2;
		else {
			warningf(false, "Can't find tty file descriptor");
			return;
		}
	}
	if ((tty_fd = ksh_dupbase(tfd, FDBASE)) < 0) {
		warningf(false, "j_ttyinit: dup of tty fd failed: %s",
			strerror(errno));
	} else if (fd_clexec(tty_fd) < 0) {
		warningf(false, "j_ttyinit: can't set close-on-exec flag: %s",
			strerror(errno));
		close(tty_fd);
		tty_fd = -1;
	} else if (init_ttystate)
		tcgetattr(tty_fd, &tty_state);
	if (do_close)
		close(tfd);
}

void
tty_close(void)
{
	if (tty_fd >= 0) {
		close(tty_fd);
		tty_fd = -1;
	}
}
