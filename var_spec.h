#if defined(VARSPEC_DEFNS)
__RCSID("$MirOS: src/bin/mksh/var_spec.h,v 1.6 2012/11/30 16:45:25 tg Exp $");
#define FN(name)			/* nothing */
#elif defined(VARSPEC_ENUMS)
#define FN(name)			V_##name,
#define F0(name)			V_##name = 0,
#elif defined(VARSPEC_ITEMS)
#define F0(name)			/* nothing */
#define FN(name)			#name,
#endif

#ifndef F0
#define F0 FN
#endif

/* NOTE: F0 are skipped for the ITEMS array, only FN generate names */

/* 0 is always V_NONE */
F0(NONE)

/* 1 and up are special variables */
FN(BASHPID)
FN(COLUMNS)
FN(EPOCHREALTIME)
#if HAVE_PERSISTENT_HISTORY
FN(HISTFILE)
#endif
FN(HISTSIZE)
FN(IFS)
FN(LINENO)
FN(LINES)
FN(OPTIND)
FN(PATH)
FN(RANDOM)
FN(SECONDS)
FN(TMOUT)
FN(TMPDIR)

#undef FN
#undef F0
#undef VARSPEC_DEFNS
#undef VARSPEC_ENUMS
#undef VARSPEC_ITEMS
