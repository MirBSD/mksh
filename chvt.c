/**	$MirBSD: src/bin/ksh/chvt.c,v 2.1 2004/12/10 18:09:41 tg Exp $ */

/*-
 * Copyright (c) 2004
 *	Thorsten "mirabile" Glaser <tg@66h.42h.de>
 *
 * Licensee is hereby permitted to deal in this work without restric-
 * tion, including unlimited rights to use, publicly perform, modify,
 * merge, distribute, sell, give away or sublicence, provided all co-
 * pyright notices above, these terms and the disclaimer are retained
 * in all redistributions or reproduced in accompanying documentation
 * or other materials provided with binary redistributions.
 *
 * Licensor hereby provides this work "AS IS" and WITHOUT WARRANTY of
 * any kind, expressed or implied, to the maximum extent permitted by
 * applicable law, but with the warranty of being written without ma-
 * licious intent or gross negligence; in no event shall licensor, an
 * author or contributor be held liable for any damage, direct, indi-
 * rect or other, however caused, arising in any way out of the usage
 * of this work, even if advised of the possibility of such damage.
 */

#include "sh.h"
#include <sys/ioctl.h>
#include "ksh_stat.h"

__RCSID("$MirBSD: src/bin/ksh/chvt.c,v 2.1 2004/12/10 18:09:41 tg Exp $");

char *
chvt(char *f)
{
#ifdef HAVE_SETSID
#ifdef TIOCSCTTY
	int fd;

	if (chown(f, 0, 0))
		return "chown";
	if (chmod(f, 0600))
		return "chmod";
#if defined(HAVE_REVOKE) && !defined(linux)
	if (revoke(f))
		return "revoke";
#endif

	if ((fd = open(f, O_RDWR)) == -1) {
		sleep(1);
		if ((fd = open(f, O_RDWR)) == -1)
			return "open";
	}
	switch (fork()) {
	case -1:
		return "fork";
	case 0:
		break;
	default:
		_exit(0);
	}
	if (setsid() == -1)
		return "setsid";
	if (ioctl(fd, TIOCSCTTY, NULL) == -1)
		return "ioctl";
	dup2(fd, 0);
	dup2(fd, 1);
	dup2(fd, 2);
	if (fd > 2)
		close(fd);

	return NULL;
#else
	return "ioctl - TIOCSCTTY not implemented";
#endif
#else
	return "setsid not implemented";
#endif
}
