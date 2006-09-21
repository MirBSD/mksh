#include "sh.h"

__RCSID("$MirOS: src/bin/mksh/compat.c,v 1.2 2006/09/21 22:03:23 tg Exp $");

#if defined(__gnu_linux__) || defined(__sun__) || defined(__CYGWIN__)
#include "setmode.c"
#endif
