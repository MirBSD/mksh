/**	$MirOS: src/bin/ksh/missing.c,v 1.1.7.1 2005/03/06 15:42:54 tg Exp $ */
/*	$OpenBSD: missing.c,v 1.5 2003/05/16 18:49:46 jsyn Exp $	*/

/*
 * Routines which may be missing on some machines
 */

#include "sh.h"
#include "ksh_stat.h"

__RCSID("$MirOS: src/bin/ksh/missing.c,v 1.1.7.1 2005/03/06 15:42:54 tg Exp $");

#ifndef HAVE_STRERROR
char *
strerror(err)
	int err;
{
	static char	buf[64];
	switch (err) {
	  case EINVAL:
		return "Invalid argument";
	  case EACCES:
		return "Permission denied";
	  case ESRCH:
		return "No such process";
	  case EPERM:
		return "Not owner";
	  case ENOENT:
		return "No such file or directory";
	  case ENOTDIR:
		return "Not a directory";
	  case ENOEXEC:
		return "Exec format error";
	  case ENOMEM:
		return "Not enough memory";
	  case E2BIG:
		return "Argument list too long";
	  default:
		shf_snprintf(buf, sizeof(buf), "Unknown system error %d", err);
		return buf;
	}
}
#endif /* !HAVE_STRERROR */

#ifdef OPENDIR_DOES_NONDIR
/* Prevent opendir() from attempting to open non-directories.  Such
 * behavior can cause problems if it attempts to open special devices...
 */
DIR *
ksh_opendir(d)
	const char *d;
{
	struct stat statb;

	if (stat(d, &statb) != 0)
		return NULL;
	if (!S_ISDIR(statb.st_mode)) {
		errno = ENOTDIR;
		return NULL;
	}
	return opendir(d);
}
#endif /* OPENDIR_DOES_NONDIR */

#ifndef HAVE_DUP2
int
dup2(oldd, newd)
	int oldd;
	int newd;
{
	int old_errno;

	if (fcntl(oldd, F_GETFL, 0) == -1)
		return -1;	/* errno == EBADF */

	if (oldd == newd)
		return newd;

	old_errno = errno;

	close(newd);	/* in case its open */

	errno = old_errno;

	return fcntl(oldd, F_DUPFD, newd);
}
#endif /* !HAVE_DUP2 */

/*
 * Random functions
 */

#ifndef HAVE_SRANDOM
#undef HAVE_RANDOM
#endif

int rnd_state;

void
rnd_seed(long newval)
{
	rnd_put(newval);
	rnd_state = 0;
}

long
rnd_get(void)
{
#ifdef HAVE_ARC4RANDOM
	if (!rnd_state)
		return arc4random() & 0x7FFF;
#endif
#ifdef HAVE_RANDOM
	return random() & 0x7FFF;
#else
	return rand();
#endif
}

void
rnd_put(long newval)
{
	long sv;

	rnd_state = 1 | rnd_get();
	sv = (rnd_get() << (newval & 7)) ^ newval;

#if defined(HAVE_ARC4RANDOM_PUSH)
	arc4random_push(sv);
#elif defined(HAVE_ARC4RANDOM_ADDRANDOM)
	arc4random_addrandom((unsigned char *)&sv, sizeof(sv));
#endif
#ifdef HAVE_ARC4RANDOM
	sv ^= arc4random();
#endif

#ifdef HAVE_RANDOM
	srandom(sv);
#else
	srand(sv);
#endif

	while (rnd_state) {
		rnd_get();
		rnd_state >>= 1;
	}
	rnd_state = 1;
}
