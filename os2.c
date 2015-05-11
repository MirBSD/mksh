/*-
 * Copyright (c) 2015
 *	KO Myung-Hun <komh@chollian.net>
 *
 * Provided that these terms and disclaimer and all copyright notices
 * are retained or reproduced in an accompanying document, permission
 * is granted to deal in this work without restriction, including un-
 * limited rights to use, publicly perform, distribute, sell, modify,
 * merge, give away, or sublicence.
 *
 * This work is provided "AS IS" and WITHOUT WARRANTY of any kind, to
 * the utmost extent permitted by applicable law, neither express nor
 * implied; without malicious intent or gross negligence. In no event
 * may a licensor, author or contributor be held liable for indirect,
 * direct, other damage, loss, or other issues arising in any way out
 * of dealing in the work, even if advised of the possibility of such
 * damage or existence of a defect, except proven that it results out
 * of said person's immediate fault when using the work as intended.
 */

#include "sh.h"

#include <klibc/startup.h>

static char *remove_trailing_dots(char *);
static int access_stat_ex(int (*)(), const char *, void *);
static int test_exec_exist(const char *, char *);
static void response(int *, const char ***);

#define RPUT(x) 													\
	do {															\
		if (new_argc >= new_alloc) {								\
			new_alloc += 20;										\
			new_argv = (const char **)realloc(new_argv,				\
											  new_alloc * sizeof(char *));\
			if (!new_argv)											\
				goto exit_out_of_memory;							\
		}															\
		new_argv[new_argc++] = x;									\
	} while (0)

#define KLIBC_ARG_RESPONSE_EXCLUDE	\
	(__KLIBC_ARG_DQUOTE | __KLIBC_ARG_WILDCARD | __KLIBC_ARG_SHELL)

static void
response(int *argcp, const char ***argvp)
{
	int i, old_argc, new_argc, new_alloc = 0;
	const char **old_argv, **new_argv;
	char *line, *l, *p;
	FILE *f;

	old_argc = *argcp; old_argv = *argvp;

	for (i = 1; i < old_argc; ++i)
		if (old_argv[i] &&
		    !(old_argv[i][-1] & KLIBC_ARG_RESPONSE_EXCLUDE) &&
		    old_argv[i][0] == '@')
			break;

	if (i >= old_argc)
		return; 					/* do nothing */

	new_argv = NULL; new_argc = 0;
	for (i = 0; i < old_argc; ++i) {
		if (i == 0 || !old_argv[i] ||
		    (old_argv[i][-1] & KLIBC_ARG_RESPONSE_EXCLUDE) ||
		    old_argv[i][0] != '@' ||
		    !(f = fopen(old_argv[i] + 1, "rt")))
			RPUT(old_argv[i]);
		else {
			long filesize;

			fseek(f, 0, SEEK_END);
			filesize = ftell(f);
			fseek(f, 0, SEEK_SET);

			line = malloc(filesize + 1);
			if (!line)
				goto exit_out_of_memory;

			line[0] = __KLIBC_ARG_NONZERO | __KLIBC_ARG_RESPONSE;
			l = line + 1;
			while (fgets(l, filesize - (l - line - 1), f)) {
				p = strchr(l, '\n');
				if (p) {
					/* if a line ends with '\',
					 * then concatenate a next line
					 */
					if (p > l && p[-1] == '\\') {
						char *p1;
						int count = 0;

						for (p1 = p - 1; p1 >= l && *p1 == '\\'; p1--)
							count++;

						if (count & 1) {
							l = p + 1;

							continue;
						}
					}

					*p = 0;
				}
				p = strdup(line);
				if (!p)
					goto exit_out_of_memory;

				RPUT(p + 1);

				l = line + 1;
			}

			free(line);

			if (ferror(f)) {
				fputs("Cannot read response file\n", stderr);
				exit(255);
			}

			fclose(f);
		}
	}

	RPUT(NULL); --new_argc;

	*argcp = new_argc; *argvp = new_argv;
	return;

exit_out_of_memory:
	fputs("Out of memory while reading response file\n", stderr);
	exit(255);
}

void os2_init(int *argcp, const char ***argvp)
{
	response(argcp, argvp);
}

/* Remove trailing dots. */
static char *
remove_trailing_dots(char *name)
{
	char *p;

	for (p = name + strlen(name); --p > name && *p == '.'; )
		/* nothing */;

	if (*p != '.' && *p != '/' && *p != '\\' && *p != ':')
		p[1] = '\0';

	return name;
}

#define REMOVE_TRAILING_DOTS(name)	\
	remove_trailing_dots(strcpy(alloca(strlen(name) + 1), name))

/* Alias of stat() */
int _std_stat(const char *, struct stat *);

/* Replacement for stat() of kLIBC which fails if there are trailing dots */
int
stat(const char *name, struct stat *buffer)
{
	return _std_stat(REMOVE_TRAILING_DOTS(name), buffer);
}

/* Alias of access() */
int _std_access(const char *, int);

/* Replacement for access() of kLIBC which fails if there are trailing dots */
int
access(const char *name, int mode)
{
	/*
	 * On OS/2 kLIBC, X_OK is set only for executable files. This prevent
	 * scripts from being executed.
	 */
	if (mode & X_OK)
		mode = (mode & ~X_OK) | R_OK;

	return _std_access(REMOVE_TRAILING_DOTS(name), mode);
}

#define MAX_X_SUFFIX_LEN	4

static const char *x_suffix_list[] =
	{"", ".ksh", ".exe", ".sh", ".cmd", ".com", ".bat", NULL};

/* Call fn() by appending executable extensions. */
static int
access_stat_ex(int (*fn)(), const char *name, void *arg)
{
	char *x_name;
	const char **x_suffix;
	int rc = -1;

	/* Have an extension ? */
	if (_getext(name))
		return fn(name, arg);

	/* Otherwise, try to append executable suffixes */
	x_name = alloc(strlen(name) + MAX_X_SUFFIX_LEN + 1, ATEMP);

	for (x_suffix = x_suffix_list; rc && *x_suffix; x_suffix++) {
		strcpy(x_name, name);
		strcat(x_name, *x_suffix);

		rc = fn(x_name, arg);
	}

	afree(x_name, ATEMP);

	return rc;
}

/* access()/search_access() version */
int
access_ex(int (*fn)(const char *, int), const char *name, int mode)
{
	return access_stat_ex(fn, name, (void *)mode);
}

/* stat() version */
int
stat_ex(const char *name, struct stat *buffer)
{
	return access_stat_ex(stat, name, buffer);
}

static int
test_exec_exist(const char *name, char *real_name)
{
	struct stat sb;

	if (stat(name, &sb) < 0 || !S_ISREG(sb.st_mode))
		return -1;

	strcpy(real_name, name);

	return 0;
}

const char *
real_exec_name(const char *name)
{
	char x_name[strlen(name) + MAX_X_SUFFIX_LEN + 1];
	const char *real_name = name;

	if (access_stat_ex(test_exec_exist, real_name, x_name) != -1)
		strdupx(real_name, x_name, ATEMP);

	return real_name;
}
