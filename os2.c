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

#define INCL_DOS
#include <os2.h>

#include "sh.h"

#include <klibc/startup.h>
#include <io.h>
#include <unistd.h>
#include <process.h>

__RCSID("$MirOS: src/bin/mksh/os2.c,v 1.1 2017/04/02 15:00:44 tg Exp $");

static char *remove_trailing_dots(char *);
static int access_stat_ex(int (*)(), const char *, void *);
static int test_exec_exist(const char *, char *);
static void response(int *, const char ***);
static char *make_response_file(char * const *);
static void env_slashify(void);
static void add_temp(const char *);
static void cleanup_temps(void);
static void cleanup(void);

#define RPUT(x) do {					\
	if (new_argc >= new_alloc) {			\
		new_alloc += 20;			\
		if (!(new_argv = realloc(new_argv,	\
		    new_alloc * sizeof(char *))))	\
			goto exit_out_of_memory;	\
	}						\
	new_argv[new_argc++] = (x);			\
} while (/* CONSTCOND */ 0)

#define KLIBC_ARG_RESPONSE_EXCLUDE	\
	(__KLIBC_ARG_DQUOTE | __KLIBC_ARG_WILDCARD | __KLIBC_ARG_SHELL)

static void
response(int *argcp, const char ***argvp)
{
	int i, old_argc, new_argc, new_alloc = 0;
	const char **old_argv, **new_argv;
	char *line, *l, *p;
	FILE *f;

	old_argc = *argcp;
	old_argv = *argvp;
	for (i = 1; i < old_argc; ++i)
		if (old_argv[i] &&
		    !(old_argv[i][-1] & KLIBC_ARG_RESPONSE_EXCLUDE) &&
		    old_argv[i][0] == '@')
			break;

	if (i >= old_argc)
		/* do nothing */
		return;

	new_argv = NULL;
	new_argc = 0;
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

			line = malloc(filesize + /* type */ 1 + /* NUL */ 1);
			if (!line) {
 exit_out_of_memory:
				fputs("Out of memory while reading response file\n", stderr);
				exit(255);
			}

			line[0] = __KLIBC_ARG_NONZERO | __KLIBC_ARG_RESPONSE;
			l = line + 1;
			while (fgets(l, (filesize + 1) - (l - (line + 1)), f)) {
				p = strchr(l, '\n');
				if (p) {
					/*
					 * if a line ends with a backslash,
					 * concatenate with the next line
					 */
					if (p > l && p[-1] == '\\') {
						char *p1;
						int count = 0;

						for (p1 = p - 1; p1 >= l &&
						    *p1 == '\\'; p1--)
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

	RPUT(NULL);
	--new_argc;

	*argcp = new_argc;
	*argvp = new_argv;
}

static void
init_extlibpath(void)
{
	const char *vars[] = {
		"BEGINLIBPATH",
		"ENDLIBPATH",
		"LIBPATHSTRICT",
		NULL
	};
	char val[512];
	int flag;

	for (flag = 0; vars[flag]; flag++) {
		DosQueryExtLIBPATH(val, flag + 1);
		if (val[0])
			setenv(vars[flag], val, 1);
	}
}

/*
 * Convert backslashes of environmental variables to forward slahes.
 * A backslash may be used as an escaped character when doing 'echo'.
 * This leads to an unexpected behavior.
 */
static void
env_slashify(void)
{
	/*
	 * PATH and TMPDIR are used by OS/2 as well. That is, they may
	 * have backslashes as a directory separator.
	 * BEGINLIBPATH and ENDLIBPATH are special variables on OS/2.
	 */
	const char *var_list[] = {
		"PATH",
		"TMPDIR",
		"BEGINLIBPATH",
		"ENDLIBPATH",
		NULL
	};
	const char **var;
	char *value;

	for (var = var_list; *var; var++) {
		value = getenv(*var);

		if (value)
			_fnslashify(value);
	}
}

void
os2_init(int *argcp, const char ***argvp)
{
	response(argcp, argvp);

	init_extlibpath();
	env_slashify();

	if (!isatty(STDIN_FILENO))
		setmode(STDIN_FILENO, O_BINARY);
	if (!isatty(STDOUT_FILENO))
		setmode(STDOUT_FILENO, O_BINARY);
	if (!isatty(STDERR_FILENO))
		setmode(STDERR_FILENO, O_BINARY);

	atexit(cleanup);
}

void
setextlibpath(const char *name, const char *val)
{
	int flag;
	char *p, *cp;

	if (!strcmp(name, "BEGINLIBPATH"))
		flag = BEGIN_LIBPATH;
	else if (!strcmp(name, "ENDLIBPATH"))
		flag = END_LIBPATH;
	else if (!strcmp(name, "LIBPATHSTRICT"))
		flag = LIBPATHSTRICT;
	else
		return;

	/* convert slashes to backslashes */
	strdupx(cp, val, ATEMP);
	for (p = cp; *p; p++) {
		if (*p == '/')
			*p = '\\';
	}

	DosSetExtLIBPATH(cp, flag);

	afree(cp, ATEMP);
}

/* remove trailing dots */
static char *
remove_trailing_dots(char *name)
{
	char *p;

	for (p = name + strlen(name); --p > name && *p == '.'; )
		/* nothing */;

	if (*p != '.' && *p != '/' && *p != '\\' && *p != ':')
		p[1] = '\0';

	return (name);
}

#define REMOVE_TRAILING_DOTS(name)	\
	remove_trailing_dots(memcpy(alloca(strlen(name) + 1), name, strlen(name) + 1))

/* alias of stat() */
extern int _std_stat(const char *, struct stat *);

/* replacement for stat() of kLIBC which fails if there are trailing dots */
int
stat(const char *name, struct stat *buffer)
{
	return (_std_stat(REMOVE_TRAILING_DOTS(name), buffer));
}

/* alias of access() */
extern int _std_access(const char *, int);

/* replacement for access() of kLIBC which fails if there are trailing dots */
int
access(const char *name, int mode)
{
	/*
	 * On OS/2 kLIBC, X_OK is set only for executable files.
	 * This prevents scripts from being executed.
	 */
	if (mode & X_OK)
		mode = (mode & ~X_OK) | R_OK;

	return (_std_access(REMOVE_TRAILING_DOTS(name), mode));
}

#define MAX_X_SUFFIX_LEN	4

static const char *x_suffix_list[] =
    { "", ".ksh", ".exe", ".sh", ".cmd", ".com", ".bat", NULL };

/* call fn() by appending executable extensions */
static int
access_stat_ex(int (*fn)(), const char *name, void *arg)
{
	char *x_name;
	const char **x_suffix;
	int rc = -1;
	size_t x_namelen = strlen(name) + MAX_X_SUFFIX_LEN + 1;

	/* otherwise, try to append executable suffixes */
	x_name = alloc(x_namelen, ATEMP);

	for (x_suffix = x_suffix_list; rc && *x_suffix; x_suffix++) {
		strlcpy(x_name, name, x_namelen);
		strlcat(x_name, *x_suffix, x_namelen);

		rc = fn(x_name, arg);
	}

	afree(x_name, ATEMP);

	return (rc);
}

/* access()/search_access() version */
int
access_ex(int (*fn)(const char *, int), const char *name, int mode)
{
	/*XXX this smells fishy --mirabilos */
	return (access_stat_ex(fn, name, (void *)mode));
}

/* stat() version */
int
stat_ex(const char *name, struct stat *buffer)
{
	return (access_stat_ex(stat, name, buffer));
}

static int
test_exec_exist(const char *name, char *real_name)
{
	struct stat sb;

	if (stat(name, &sb) < 0 || !S_ISREG(sb.st_mode))
		return (-1);

	/* safe due to calculations in real_exec_name() */
	memcpy(real_name, name, strlen(name) + 1);

	return (0);
}

const char *
real_exec_name(const char *name)
{
	char x_name[strlen(name) + MAX_X_SUFFIX_LEN + 1];
	const char *real_name = name;

	if (access_stat_ex(test_exec_exist, real_name, x_name) != -1)
		/*XXX memory leak */
		strdupx(real_name, x_name, ATEMP);

	return (real_name);
}

/* OS/2 can process a command line up to 32 KiB */
#define MAX_CMD_LINE_LEN 32768

/* make a response file to pass a very long command line */
static char *
make_response_file(char * const *argv)
{
	char rsp_name_arg[] = "@mksh-rsp-XXXXXX";
	char *rsp_name = &rsp_name_arg[1];
	int arg_len = 0;
	int i;

	for (i = 0; argv[i]; i++)
		arg_len += strlen(argv[i]) + 1;

	/*
	 * If a length of command line is longer than MAX_CMD_LINE_LEN, then
	 * use a response file. OS/2 cannot process a command line longer
	 * than 32K. Of course, a response file cannot be recognised by a
	 * normal OS/2 program, that is, neither non-EMX or non-kLIBC. But
	 * it cannot accept a command line longer than 32K in itself. So
	 * using a response file in this case, is an acceptable solution.
	 */
	if (arg_len > MAX_CMD_LINE_LEN) {
		int fd;
		char *result;

		if ((fd = mkstemp(rsp_name)) == -1)
			return (NULL);

		/* write all the arguments except a 0th program name */
		for (i = 1; argv[i]; i++) {
			write(fd, argv[i], strlen(argv[i]));
			write(fd, "\n", 1);
		}

		close(fd);
		add_temp(rsp_name);
		strdupx(result, rsp_name_arg, ATEMP);
		return (result);
	}

	return (NULL);
}

/* alias of execve() */
extern int _std_execve(const char *, char * const *, char * const *);

/* replacement for execve() of kLIBC */
int
execve(const char *name, char * const *argv, char * const *envp)
{
	const char *exec_name;
	FILE *fp;
	char sign[2];
	char *rsp_argv[3];
	char *rsp_name_arg;
	int pid;
	int status;
	int fd;
	int rc;

	/*
	 * #! /bin/sh : append .exe
	 * extproc sh : search sh.exe in PATH
	 */
	exec_name = search_path(name, path, X_OK, NULL);
	if (!exec_name) {
		errno = ENOENT;
		return (-1);
	}

	/*-
	 * kLIBC execve() has problems when executing scripts.
	 * 1. it fails to execute a script if a directory whose name
	 *    is same as an interpreter exists in a current directory.
	 * 2. it fails to execute a script not starting with sharpbang.
	 * 3. it fails to execute a batch file if COMSPEC is set to a shell
	 *    incompatible with cmd.exe, such as /bin/sh.
	 * And ksh process scripts more well, so let ksh process scripts.
	 */
	errno = 0;
	if (!(fp = fopen(exec_name, "rb")))
		errno = ENOEXEC;

	if (!errno && fread(sign, 1, sizeof(sign), fp) != sizeof(sign))
		errno = ENOEXEC;

	if (fp && fclose(fp))
		errno = ENOEXEC;

	if (!errno &&
	    !((sign[0] == 'M' && sign[1] == 'Z') ||
	      (sign[0] == 'N' && sign[1] == 'E') ||
	      (sign[0] == 'L' && sign[1] == 'X')))
		errno = ENOEXEC;

	if (errno == ENOEXEC)
		return (-1);

	rsp_name_arg = make_response_file(argv);

	if (rsp_name_arg) {
		rsp_argv[0] = argv[0];
		rsp_argv[1] = rsp_name_arg;
		rsp_argv[2] = NULL;

		argv = rsp_argv;
	}

	pid = spawnve(P_NOWAIT, exec_name, argv, envp);

	afree(rsp_name_arg, ATEMP);

	if (pid == -1) {
		cleanup_temps();

		return (-1);
	}

	/* close all opened handles */
	for (fd = 0; fd < NUFILE; fd++) {
		if (fcntl(fd, F_GETFD) == -1)
			continue;

		close(fd);
	}

	while ((rc = waitpid(pid, &status, 0)) < 0 && errno == EINTR)
		/* nothing */;

	cleanup_temps();

	/* Is this possible? And is this right? */
	if (rc == -1)
		return (-1);

	if (WIFSIGNALED(status))
		_exit(ksh_sigmask(WTERMSIG(status)));

	_exit(WEXITSTATUS(status));
}

static struct temp *templist = NULL;

static void
add_temp(const char *name)
{
	struct temp *tp;

	tp = alloc(offsetof(struct temp, tffn[0]) + strlen(name) + 1, APERM);
	memcpy(tp->tffn, name, strlen(name) + 1);
	tp->next = templist;
	templist = tp;
}

/* alias of unlink() */
extern int _std_unlink(const char *);

/*
 * Replacement for unlink() of kLIBC not supporting to remove files used by
 * another processes.
 */
int
unlink(const char *name)
{
	int rc;

	rc = _std_unlink(name);
	if (rc == -1 && errno != ENOENT)
		add_temp(name);

	return (rc);
}

static void
cleanup_temps(void)
{
	struct temp *tp;
	struct temp **tpnext;

	for (tpnext = &templist, tp = templist; tp; tp = *tpnext) {
		if (_std_unlink(tp->tffn) == 0 || errno == ENOENT) {
			*tpnext = tp->next;
			afree(tp, APERM);
		} else {
			tpnext = &tp->next;
		}
	}
}

static void
cleanup(void)
{
	cleanup_temps();
}
