/**	$MirBSD: version.c,v 1.15 2004/10/28 16:35:03 tg Exp $ */
/*	$OpenBSD: version.c,v 1.12 1999/07/14 13:37:24 millert Exp $	*/

/*-
 * Copyright (c) 2004
 *	Thorsten "mirabile" Glaser <tg@66h.42h.de>
 *
 * Licensee is hereby permitted to deal in this work without restric-
 * tion, including unlimited rights to use, publically perform, modi-
 * fy, merge, distribute, sell, give away or sublicence, provided the
 * above copyright notices, these terms and the disclaimer are retai-
 * ned in all redistributions, or reproduced in accompanying documen-
 * tation or other materials provided with binary redistributions.
 *
 * Licensor hereby provides this work "AS IS" and WITHOUT WARRANTY of
 * any kind, expressed or implied, to the maximum extent permitted by
 * applicable law, but with the warranty of being written without ma-
 * licious intent or gross negligence; in no event shall licensor, an
 * author or contributor be held liable for any damage, direct, indi-
 * rect or other, however caused, arising in any way out of the usage
 * of covered work, even if advised of the possibility of such damage.
 *-
 * value of $KSH_VERSION (or $SH_VERSION)
 */

#include "sh.h"

const char ksh_version[] =
	"@(#)PD KSH v5.2.14 MirOS $Revision: 1.15 $ in "
#ifdef MIRBSD_NATIVE
	"native "
#endif
#ifdef KSH
	"KSH"
#else
	"POSIX"
#endif
	" mode"
#ifdef MKSH
	" as mksh"
#endif
	;
