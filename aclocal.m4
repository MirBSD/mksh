dnl $MirBSD: src/bin/ksh/aclocal.m4,v 2.5 2004/12/31 18:41:46 tg Exp $
dnl-
dnl Copyright (c) 2004
dnl	Thorsten "mirabile" Glaser <tg@66h.42h.de>
dnl
dnl Licensee is hereby permitted to deal in this work without restric-
dnl tion, including unlimited rights to use, publicly perform, modify,
dnl merge, distribute, sell, give away or sublicence, provided all co-
dnl pyright notices above, these terms and the disclaimer are retained
dnl in all redistributions or reproduced in accompanying documentation
dnl or other materials provided with binary redistributions.
dnl
dnl Licensor hereby provides this work "AS IS" and WITHOUT WARRANTY of
dnl any kind, expressed or implied, to the maximum extent permitted by
dnl applicable law, but with the warranty of being written without ma-
dnl licious intent or gross negligence; in no event shall licensor, an
dnl author or contributor be held liable for any damage, direct, indi-
dnl rect or other, however caused, arising in any way out of the usage
dnl of this work, even if advised of the possibility of such damage.
dnl-
dnl Based upon code by:
dnl Copyright (C) 1996, Memorial University of Newfoundland.
dnl-
dnl
dnl
dnl Like AC_CHECK_TYPE(), only
dnl	- user gets to specify header file(s) in addition to the default
dnl	  headers (<sys/types.h> and <stdlib.h>)
dnl	- user gets to specify the message
dnl	- word boundary checks are put at beginning/end of pattern
dnl	  (ie, \<pattern\>)
dnl	- default argument is optional
dnl uses ac_cv_type_X 'cause this is used in other autoconf macros...
dnl KSH_CHECK_H_TYPE(type, message, header files, default)
AC_DEFUN(KSH_CHECK_H_TYPE,
 [AC_CACHE_CHECK($2, ac_cv_type_$1,
    [AC_EGREP_CPP([(^|[^a-zA-Z0-9_])]$1[([^a-zA-Z0-9_]|\$)],
      [#include <sys/types.h>
#if STDC_HEADERS
#include <stdlib.h>
#endif
$3
      ], ac_cv_type_$1=yes, ac_cv_type_$1=no)])
  ifelse($#, 4, [if test $ac_cv_type_$1 = no; then
      AC_DEFINE($1, $4)
  fi
  ])dnl
 ])dnl
dnl
dnl
dnl Check for memmove and if not found, check for bcopy.  AC_CHECK_FUNCS()
dnl not used 'cause it confuses some compilers that have memmove/bcopy builtin;
dnl Also want to check if the function deals with overlapping src/dst properly.
AC_DEFUN(KSH_MEMMOVE,
 [AC_CACHE_CHECK(for working memmove, ksh_cv_func_memmove,
    [AC_TRY_RUN([
#ifdef HAVE_STRING_H
# include <string.h>
#else
# include <strings.h>
#endif
#ifdef HAVE_MEMORY_H
# include <memory.h>
#endif
        int
        main()
        {
	  char buf[16];
	  strcpy(buf, "abcdefABCDEF");
	  memmove(buf + 4, buf, 6);
	  if (strcmp(buf, "abcdabcdefEF"))
	    exit(1);
	  memmove(buf, buf + 4, 6);
	  if (strcmp(buf, "abcdefcdefEF"))
	    exit(2);
	  exit(0);
	  return 0;
	}],
       ksh_cv_func_memmove=yes, ksh_cv_func_memmove=no,
       AC_MSG_WARN(assuming memmove broken); ksh_cv_func_memmove=no)])
  if test $ksh_cv_func_memmove = yes; then
    AC_DEFINE(HAVE_MEMMOVE)
  else
    AC_CACHE_CHECK(for working bcopy, ksh_cv_func_bcopy,
      [AC_TRY_RUN([
#ifdef HAVE_STRING_H
# include <string.h>
#else
# include <strings.h>
#endif
#ifdef HAVE_MEMORY_H
# include <memory.h>
#endif
	  int
	  main()
	  {
	    char buf[16];
	    strcpy(buf, "abcdefABCDEF");
	    bcopy(buf, buf + 4, 6);
	    if (strcmp(buf, "abcdabcdefEF"))
	      exit(1);
	    bcopy(buf + 4, buf, 6);
	    if (strcmp(buf, "abcdefcdefEF"))
	      exit(2);
	    exit(0);
	  }],
	 ksh_cv_func_bcopy=yes, ksh_cv_func_bcopy=no,
         AC_MSG_WARN(assuming bcopy broken); ksh_cv_func_bcopy=no)])
    if test $ksh_cv_func_bcopy = yes; then
      AC_DEFINE(HAVE_BCOPY)
    fi
  fi
 ])dnl
dnl
dnl
dnl Check for sigsetjmp()/siglongjmp() and _setjmp()/_longjmp() pairs.
dnl Can't use simple library check as QNX 422 has _setjmp() but not _longjmp()
dnl (go figure).
AC_DEFUN(KSH_SETJMP,
 [AC_CACHE_CHECK(for sigsetjmp()/siglongjmp(), ksh_cv_func_sigsetjmp,
    [AC_TRY_LINK([], [sigsetjmp(); siglongjmp()],
       ksh_cv_func_sigsetjmp=yes, ksh_cv_func_sigsetjmp=no)])
  if test $ksh_cv_func_sigsetjmp = yes; then
    AC_DEFINE(HAVE_SIGSETJMP)
  else
    AC_CACHE_CHECK(for _setjmp()/_longjmp(), ksh_cv_func__setjmp,
      [AC_TRY_LINK([], [_setjmp(); _longjmp();],
	 ksh_cv_func__setjmp=yes, ksh_cv_func__setjmp=no)])
    if test $ksh_cv_func__setjmp = yes; then
      AC_DEFINE(HAVE__SETJMP)
    fi
  fi
 ])dnl
dnl
dnl
dnl Check for memset function.   AC_CHECK_FUNCS() not used 'cause it confuses
dnl some compilers that have memset builtin.
AC_DEFUN(KSH_MEMSET,
 [AC_CACHE_CHECK(for memset, ksh_cv_func_memset,
    [AC_TRY_LINK([
#ifdef HAVE_STRING_H
# include <string.h>
#else
# include <strings.h>
#endif
#ifdef HAVE_MEMORY_H
# include <memory.h>
#endif
       ], [
       char buf[16]; memset(buf, 'x', 7); printf("%7s", buf);],
       ksh_cv_func_memset=yes, ksh_cv_func_memset=no)])
  if test $ksh_cv_func_memset = yes; then
    AC_DEFINE(HAVE_MEMSET)
  fi
 ])dnl
dnl
dnl
dnl Check for rlim_t in a few places, and if not found, figure out the
dnl size rlim_t should be by looking at struct rlimit.rlim_cur.
AC_DEFUN(KSH_RLIM_CHECK,
 [KSH_CHECK_H_TYPE(rlim_t, for rlim_t in <sys/types.h> and <sys/resource.h>,
   [#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif])dnl
  if test $ac_cv_type_rlim_t = no; then
    AC_MSG_CHECKING(what to set rlim_t to)
    if test $ac_cv_header_sys_resource_h = yes; then
      AC_CACHE_VAL(ksh_cv_rlim_check,
	[AC_TRY_RUN([
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
	    main()
	    {
	      struct rlimit rl;
	      if (sizeof(rl.rlim_cur) == sizeof(quad_t))
		exit(0);
	      exit(1);
	    }
	  ], ksh_cv_rlim_check=quad_t, ksh_cv_rlim_check=long,
	  AC_MSG_ERROR(cannot determine type for rlimt_t when cross compiling)
	  )])dnl
    else
      ksh_cv_rlim_check=long
    fi
    AC_MSG_RESULT($ksh_cv_rlim_check)
    AC_DEFINE_UNQUOTED(rlim_t, $ksh_cv_rlim_check)
  fi
 ])dnl
dnl
dnl
AC_DEFUN(KSH_DEV_FD,
 [AC_CACHE_CHECK(if you have /dev/fd/n, ksh_cv_dev_fd,
    [AC_TRY_RUN([
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
	  main()
	  {
	    struct stat s1, s2;
	    FILE *fp1, *fp2;
	    char *file = "conftest.file";
	    char devfd[32];

	    if (!(fp1 = fopen(file, "w")))
	      exit(1);
	    if (fstat(fileno(fp1), &s1) < 0)
	      exit(2);
	    sprintf(devfd, "/dev/fd/%d", fileno(fp1));
	    if (!(fp2 = fopen(devfd, "w")))
	      exit(3);
	    if (fstat(fileno(fp2), &s2) < 0)
	      exit(4);
	    if (s1.st_dev != s2.st_dev || s1.st_ino != s2.st_ino)
	      exit(5);
	    exit(0);
	  }
	], ksh_cv_dev_fd=yes, ksh_cv_dev_fd=no,
	AC_MSG_WARN(cannot determine if you have /dev/fd support, assuming not)
	ksh_cv_dev_fd=no)])
  if test $ksh_cv_dev_fd = yes; then
    AC_DEFINE(HAVE_DEV_FD)
  fi
 ])dnl
dnl
dnl
dnl  Check for sys_siglist[] declaration and existence.
AC_DEFUN(KSH_SYS_SIGLIST,
 [AC_DECL_SYS_SIGLIST
  if test ac_cv_decl_sys_siglist = yes; then
    AC_DEFINE(HAVE_SYS_SIGLIST)
  else
    AC_CACHE_CHECK(for sys_siglist in library, ksh_cv_var_sys_siglist,
      [AC_TRY_LINK(, [
	  extern char *sys_siglist[];
	  char *p = sys_siglist[2];
	  if (p)
		  return 12;
	  ], ksh_cv_var_sys_siglist=yes, ksh_cv_var_sys_siglist=no)])
    if test $ksh_cv_var_sys_siglist = yes; then
      AC_DEFINE(HAVE_SYS_SIGLIST)
    fi
  fi
 ])dnl
dnl
dnl
dnl  Check for sys_errlist[] declaration and existence.
AC_DEFUN(KSH_SYS_ERRLIST,
 [AC_CACHE_CHECK(for sys_errlist declaration in errno.h, ksh_cv_decl_sys_errlist,
  [AC_TRY_COMPILE([#include <errno.h>],
    [char *msg = *(sys_errlist + 1); if (msg && *msg) return 12; ],
    ksh_cv_decl_sys_errlist=yes, ksh_cv_decl_sys_errlist=no)])
  if test $ksh_cv_decl_sys_errlist = yes; then
    AC_DEFINE(SYS_ERRLIST_DECLARED)
    AC_DEFINE(HAVE_SYS_ERRLIST)
  else
    AC_CACHE_CHECK(for sys_errlist in library, ksh_cv_var_sys_errlist,
      [AC_TRY_LINK(, [
	    extern char *sys_errlist[];
	    extern int sys_nerr;
	    char *p;
	    p = sys_errlist[sys_nerr - 1];
	    if (p) return 12;
	  ], ksh_cv_var_sys_errlist=yes, ksh_cv_var_sys_errlist=no)])
    if test $ksh_cv_var_sys_errlist = yes; then
      AC_DEFINE(HAVE_SYS_ERRLIST)
    fi
  fi
 ])dnl
dnl
dnl
dnl  Check if time() declared in time.h
AC_DEFUN(KSH_TIME_DECLARED,
 [AC_CACHE_CHECK(time() declaration in time.h, ksh_cv_time_delcared,
    [AC_TRY_COMPILE([#include <sys/types.h>
#include <time.h>], [time_t (*f)() = time; if (f) return 12;],
      ksh_cv_time_delcared=yes, ksh_cv_time_delcared=no)])
  if test $ksh_cv_time_delcared = yes; then
    AC_DEFINE(TIME_DECLARED)
  fi
 ])dnl
dnl
dnl
AC_DEFUN(KSH_C_VOID,
 [AC_CACHE_CHECK(if compiler understands void, ksh_cv_c_void,
    [AC_TRY_COMPILE(
      [
	void foo() { }
	/* Some compilers (old pcc ones) like "void *a;", but a can't be used */
	void *bar(a) void *a; { int *b = (int *) a; *b = 1; return a; }
      ], , ksh_cv_c_void=yes, ksh_cv_c_void=no)])
  if test $ksh_cv_c_void = yes; then
    :
  else
    AC_DEFINE(void, char)
  fi
 ])dnl
dnl
dnl
dnl Early MIPS compilers (used in Ultrix 4.2) don't like
dnl "int x; int *volatile a = &x; *a = 0;"
AC_DEFUN(KSH_C_VOLATILE,
 [AC_CACHE_CHECK(if compiler understands volatile, ksh_cv_c_volatile,
    [AC_TRY_COMPILE([int x, y, z;],
      [volatile int a; int * volatile b = x ? &y : &z;
      /* Older MIPS compilers (eg., in Ultrix 4.2) don't like *b = 0 */
      *b = 0;], ksh_cv_c_volatile=yes, ksh_cv_c_volatile=no)])
  if test $ksh_cv_c_volatile = yes; then
    :
  else
    AC_DEFINE(volatile, )
  fi
 ])dnl
dnl
dnl
dnl Check if C compiler understands gcc's __attribute((...)).
dnl checks for noreturn, const, and format(type,fmt,param), also checks
dnl that the compiler doesn't die when it sees an unknown attribute (this
dnl isn't perfect since gcc doesn't parse unknown attributes with parameters)
AC_DEFUN(KSH_C_FUNC_ATTR,
 [AC_CACHE_CHECK(if C compiler groks __attribute__(( .. )), ksh_cv_c_func_attr,
    [AC_TRY_COMPILE([
#include <stdarg.h>
void test_fmt(char *fmt, ...) __attribute__((format(printf, 1, 2)));
void test_fmt(char *fmt, ...) { return; }
int test_cnst(int) __attribute__((const));
int test_cnst(int x) { return x + 1; }
void test_nr() __attribute__((noreturn));
void test_nr() { exit(1); }
void test_uk() __attribute__((blah));
void test_uk() { return; }
      ], [test_nr("%d", 10); test_cnst(2); test_uk(); test_nr(); ],
      ksh_cv_c_func_attr=yes, ksh_cv_c_func_attr=no)])
  if test $ksh_cv_c_func_attr = yes; then
    AC_DEFINE(HAVE_GCC_FUNC_ATTR)
  fi
 ])dnl
dnl
dnl
dnl Check if dup2() does not clear the close on exec flag
AC_DEFUN(KSH_DUP2_CLEXEC_CHECK,
 [AC_CACHE_CHECK([if dup2() works (ie, resets the close-on-exec flag)], ksh_cv_dup2_clexec_ok,
    [AC_TRY_RUN([
#include <sys/types.h>
#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif /* HAVE_FCNTL_H */
#ifndef F_GETFD
# define F_GETFD 1
#endif
#ifndef F_SETFD
# define F_SETFD 2
#endif
#ifndef O_RDONLY
# define O_RDONLY 0
#endif
/* On some systems (Ultrix 2.1..4.2 (and more?)), dup2() does not clear
   the close on exec flag */
main()
{
  int fd1, fd2;
  fd1 = open("/dev/null", O_RDONLY);
  if (fcntl(fd1, F_SETFD, 1) < 0)
    exit(1);
  fd2 = dup2(fd1, fd1 + 1);
  if (fd2 < 0)
    exit(2);
  exit(fcntl(fd2, F_GETFD, 0) == 0 ? 0 : 3);
}
     ], ksh_cv_dup2_clexec_ok=yes, ksh_cv_dup2_clexec_ok=no,
     AC_MSG_WARN(cannot test if dup2 is broken when cross compiling - assuming it is)
     ksh_cv_dup2_clexec_ok=no)])
  if test $ksh_cv_dup2_clexec_ok = no; then
    AC_DEFINE(DUP2_BROKEN)
  fi
 ])dnl
dnl
dnl
dnl Check type of signal routines (posix, 4.2bsd, 4.1bsd or v7)
AC_DEFUN(KSH_SIGNAL_CHECK,
 [AC_CACHE_CHECK(flavour of signal routines, ksh_cv_signal_check,
    [AC_TRY_LINK([#include <signal.h>], [
	sigset_t ss;
	struct sigaction sa;
	sigemptyset(&ss); sigsuspend(&ss);
	sigaction(SIGINT, &sa, (struct sigaction *) 0);
	sigprocmask(SIG_BLOCK, &ss, (sigset_t *) 0);
      ], ksh_cv_signal_check=posix,
      AC_TRY_LINK([#include <signal.h>], [
	  int mask = sigmask(SIGINT);
	  sigsetmask(mask); sigblock(mask); sigpause(mask);
	], ksh_cv_signal_check=bsd42,
        AC_TRY_LINK([#include <signal.h>
			RETSIGTYPE foo() { }],
	  [
	    int mask = sigmask(SIGINT);
	    sigset(SIGINT, foo); sigrelse(SIGINT);
	    sighold(SIGINT); sigpause(SIGINT);
	  ], ksh_cv_signal_check=bsd41, ksh_cv_signal_check=v7)))])
  if test $ksh_cv_signal_check != posix; then
    AC_MSG_WARN(no posix signals)
  fi
 ])dnl
dnl
dnl
dnl What kind of process groups: POSIX, BSD, SYSV or none
dnl	BSD uses setpgrp(pid, pgrp), getpgrp(pid)
dnl	POSIX uses setpid(pid, pgrp), getpgrp(void)
dnl	SYSV uses setpgrp(void), getpgrp(void)
dnl Checks for BSD first since the posix test may succeed on BSDish systems
dnl (depends on what random value gets passed to getpgrp()).
AC_DEFUN(KSH_PGRP_CHECK,
 [AC_CACHE_CHECK(flavour of pgrp routines, ksh_cv_pgrp_check,
    [AC_TRY_RUN([
/* Check for BSD process groups */
#include <signal.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif /* HAVE_UNISTD_H */
	main()
	{
	  int ecode = 0, child = fork();
	  if (child < 0)
	    exit(1);
	  if (child == 0) {
	    signal(SIGTERM, SIG_DFL); /* just to make sure */
	    sleep(10);
	    exit(9);
	  }
	  if (setpgrp(child, child) < 0)
	    ecode = 2;
	  else if (getpgrp(child) != child)
	    ecode = 3;
	  kill(child, SIGTERM);
	  exit(ecode);
	}
       ], ksh_cv_pgrp_check=bsd,
       [AC_TRY_RUN([
/* Check for POSIX process groups */
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif /* HAVE_UNISTD_H */
	  main()
	  {
	    int child;
	    int n, p1[2], p2[2];
	    char buf[1];
	    if (pipe(p1) < 0 || pipe(p2) < 0)
	      exit(1);
	    if ((child = fork()) < 0)
	      exit(2);
	    if (child == 0) {
	      n = read(p1[0], buf, sizeof(buf)); /* wait for parent to setpgid */
	      buf[0] = (n != 1 ? 10 : (getpgrp() != getpid() ? 11 : 0));
	      if (write(p2[1], buf, sizeof(buf)) != 1)
		exit(12);
	      exit(0);
	    }
	    if (setpgid(child, child) < 0)
	      exit(3);
	    if (write(p1[1], buf, 1) != 1)
	      exit(4);
	    if (read(p2[0], buf, 1) != 1)
	      exit(5);
	    exit((int) buf[0]);
	  }
	 ], ksh_cv_pgrp_check=posix,
	 [AC_TRY_RUN([
/* Check for SYSV process groups */
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif /* HAVE_UNISTD_H */
	    main()
	    {
	      int child;
	      int n, p[2];
	      char buf[1];
	      if (pipe(p) < 0)
		exit(1);
	      if ((child = fork()) < 0)
		exit(2);
	      if (child == 0) {
		buf[0] = (setpgrp() < 0 ? 10 : (getpgrp() != getpid() ? 11 : 0));
		if (write(p[1], buf, sizeof(buf)) != 1)
		  exit(11);
		exit(0);
	      }
	      if (read(p[0], buf, 1) != 1)
		exit(3);
	      exit((int) buf[0]);
	    }
	   ], ksh_cv_pgrp_check=sysv, ksh_cv_pgrp_check=none,
	   AC_MSG_ERROR(cannot taste pgrp routines when cross compiling))],
	   AC_MSG_ERROR(cannot taste pgrp routines when cross compiling))],
	   AC_MSG_ERROR(cannot taste pgrp routines when cross compiling))])
  if test $ksh_cv_pgrp_check = bsd; then
    AC_DEFINE(BSD_PGRP)
  elif test $ksh_cv_pgrp_check = posix; then
    AC_DEFINE(POSIX_PGRP)
  elif test $ksh_cv_pgrp_check = sysv; then
    AC_DEFINE(SYSV_PGRP)
  else
    AC_DEFINE(NO_PGRP)
  fi
 ])dnl
dnl
dnl
dnl Check if the pgrp of setpgrp() can't be the pid of a zombie process.
dnl On some systems, the kernel doesn't count zombie processes when checking
dnl if a process group is valid, which can cause problems in creating the
dnl pipeline "cmd1 | cmd2": if cmd1 can die (and go into the zombie state)
dnl before cmd2 is started, the kernel doesn't allow the setpgrp() for cmd2
dnl to succeed.  This test defines NEED_PGRP_SYNC if the kernel has this bug.
dnl (pgrp_sync test doesn't mean much if don't have bsd or posix pgrps)
AC_DEFUN(KSH_PGRP_SYNC,
 [AC_REQUIRE([KSH_PGRP_CHECK])dnl
  if test $ksh_cv_pgrp_check = bsd || test $ksh_cv_pgrp_check = posix ; then
   AC_CACHE_CHECK(if process group synchronization is required, ksh_cv_need_pgrp_sync,
      [AC_TRY_RUN([
	  main()
	  {
#ifdef POSIX_PGRP
#  define getpgID()	getpgrp()
#else
#  define getpgID()	getpgrp(0)
#  define setpgid(x,y)	setpgrp(x,y)
#endif
	    int pid1, pid2, fds[2];
	    int status;
	    char ok;
	    switch (pid1 = fork()) {
	      case -1:
		exit(1);
	      case 0:
		setpgid(0, getpid());
		exit(0);
	    }
	    setpgid(pid1, pid1);
	    sleep(2);	/* let first child die */
	    if (pipe(fds) < 0)
	      exit(2);
	    switch (pid2 = fork()) {
	      case -1:
		exit(3);
	      case 0:
		setpgid(0, pid1);
		ok = getpgID() == pid1;
		write(fds[1], &ok, 1);
		exit(0);
	    }
	    setpgid(pid2, pid1);
	    close(fds[1]);
	    if (read(fds[0], &ok, 1) != 1)
	      exit(4);
	    wait(&status);
	    wait(&status);
	    exit(ok ? 0 : 5);
	  }
        ], ksh_cv_need_pgrp_sync=no, ksh_cv_need_pgrp_sync=yes,
	AC_MSG_WARN(cannot test if pgrp synchronization needed when cross compiling - assuming it is)
        ksh_cv_need_pgrp_sync=yes)])
    if test $ksh_cv_need_pgrp_sync = yes; then
      AC_DEFINE(NEED_PGRP_SYNC)
    fi
  fi
 ])dnl
dnl
dnl
dnl Check to see if opendir will open non-directories (not a nice thing)
AC_DEFUN(KSH_OPENDIR_CHECK,
 [AC_CACHE_CHECK(if opendir() fails to open non-directories, ksh_cv_opendir_ok,
    [AC_TRY_RUN([
#include <stdio.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif /* HAVE_UNISTD_H */
#if defined(HAVE_DIRENT_H)
# include <dirent.h>
#else
# define dirent direct
# ifdef SYSNDIR
#  include <sys/ndir.h>
# endif /* SYSNDIR */
# ifdef SYSDIR
#  include <sys/dir.h>
# endif /* SYSDIR */
# ifdef NDIR
#  include <ndir.h>
# endif /* NDIR */
#endif /* DIRENT */
	main()
	{
	  int i, ret = 0;
	  FILE *fp;
	  char *fname = "conftestod", buf[256];
	  for (i = 0; i < sizeof(buf); i++) /* memset(buf, 0, sizeof(buf)) */
	    buf[i] = 0;
	  unlink(fname); /* paranoia */
	  i = ((fp = fopen(fname, "w")) == (FILE *) 0 && (ret = 1))
	       || (fwrite(buf, sizeof(buf), 1, fp) != 1 && (ret = 2))
	       || (fclose(fp) == EOF && (ret = 3))
	       || (opendir(fname) && (ret = 4))
	       || (opendir("/dev/null") && (ret = 5));
	  unlink(fname);
	  exit(ret);
	}
      ], ksh_cv_opendir_ok=yes, ksh_cv_opendir_ok=no,
      AC_MSG_WARN(cannot test if opendir opens non-directories when cross compiling - assuming it does)
      ksh_cv_opendir_ok=no)])
  if test $ksh_cv_opendir_ok = no; then
    AC_DEFINE(OPENDIR_DOES_NONDIR)
  fi
 ])dnl
dnl
dnl
dnl Like AC_HAVE_HEADER(unistd.h) but only defines HAVE_UNISTD_H if
dnl the header file is sane (MIPS RISC/os 5.0 (and later?) has a unistd.h
dnl in the bsd43 environ that is incorrect - it defines POSIX_VERSION even
dnl though its non-posix).
AC_DEFUN(KSH_UNISTD_H,
 [AC_CACHE_CHECK(for sane unistd.h, ksh_cv_header_unistd,
    [AC_TRY_COMPILE([
#include <unistd.h>
#if defined(_POSIX_VERSION)
# include <sys/types.h>
# include <dirent.h> /* _POSIX_VERSION => HAVE_DIRENT_H test not needed */
#endif
      ], , ksh_cv_header_unistd=yes, ksh_cv_header_unistd=no)])
  if test $ksh_cv_header_unistd = yes; then
    AC_DEFINE(HAVE_UNISTD_H)
  fi
 ])dnl
dnl
dnl
dnl Several OSes need to be detected and symbols defined so the shell can
dnl deal with them.  This is a bit kludgy, but...
dnl Currently tests for:
dnl	AIX, ISC (Interactive systems corp), MINIX,
dnl	SCO (santa cruz operation), NEXT, Interix/MS Services for Unix
dnl DO NOT USE with AC_AIX, AC_MINIX or AC_ISC_POSIX tests as these are
dnl incorperated in this test.
AC_DEFUN(KSH_OS_TYPE,
 [AC_BEFORE([$0], [AC_TRY_COMPILE])dnl
  AC_BEFORE([$0], [AC_TRY_LINK])dnl
  AC_BEFORE([$0], [AC_TRY_RUN])dnl
  AC_CACHE_CHECK(if this is a problematic os, ksh_cv_os_type,
    [ ksh_cv_os_type=no
      # Some tests below add -C to CPPFLAGS
      saveCPPFLAGS="$CPPFLAGS"
      for i in AIX ISC MINIX SCO TITANOS NEXT HPUX Interix; do
	case $i in	#((
	  AIX)
	    AC_EGREP_CPP(yes,
	      [
#ifdef _AIX
yes
#endif
	       ], ksh_cv_os_type=$i)
	    ;;	#(
	  ISC)
	    # Both native ISC cpp and gcc understand this (leave comments in)
	    CPPFLAGS="$CPPFLAGS -C"
	    #XXX grep part won't work if cross-compiling...
	    AC_EGREP_CPP(INTERACTIVE Systems Corporation,
	      [#include <unistd.h>],
		[if grep _POSIX_VERSION /usr/include/sys/unistd.h > /dev/null 2>&1; then
		  ksh_cv_os_type="$i-posix"
		else
		  ksh_cv_os_type=$i
		fi])dnl
	    CPPFLAGS="$saveCPPFLAGS"
	    ;;	#(
	  MINIX)
	    AC_CHECK_HEADER(minix/config.h, ksh_cv_os_type=$i)dnl
	    AC_MSG_CHECKING(for problematic OS continues)
	    ;;	#(
	  SCO)
	    # Both native SCO cpp and gcc understand this (leave comments in)
	    CPPFLAGS="$CPPFLAGS -C"
	    AC_EGREP_CPP(The Santa Cruz Operation,
	      [#include <unistd.h>], ksh_cv_os_type=$i)dnl
	    CPPFLAGS="$saveCPPFLAGS"
	    ;;	#(
	  TITANOS)
	    AC_EGREP_CPP(YesTitan,
	      [
#if defined(titan) || defined(_titan) || defined(__titan)
YesTitan
#endif
	       ], ksh_cv_os_type=$i)dnl
	    ;;	#(
	  NEXT)
	    #
	    # NeXT 3.2 (other versions?) - cc -E doesn't work and /lib/cpp
	    # doesn't define things that need defining, so tests that rely
	    # on $CPP will break.
	    #
	    # Hmmm - can't safely use CPP to test for NeXT defines, so have
	    # to use a program that won't compile on a NeXT and one that will
	    # only compile on a NeXT...
	    AC_TRY_COMPILE([], [
	        #if defined(__NeXT) || defined(NeXT)
		  this is a NeXT box and the compile should fail
	        #endif
	      ], , AC_TRY_COMPILE([], [
		  #if !defined(__NeXT) && !defined(NeXT)
		    this is NOT a NeXT box and the compile should fail
		  #endif
		], ksh_cv_os_type=$i))dnl
	    ;;	#(
	  HPUX)
	    AC_EGREP_CPP(yes,
	      [
#ifdef __hpux
yes
#endif
	       ], ksh_cv_os_type=$i)
	    ;;	#(
	  Interix)
	    AC_EGREP_CPP(is_interix_sfu,
	      [
#include <interix/interix.h>
#if defined(__MirInterix__)
#elif defined(_INTERIX_INTERIX_H) || defined(WIN_REG_NONE)
is_interix_sfu
#endif
	       ], ksh_cv_os_type=$i)dnl
	    ;;	#(
	esac		#))
	test $ksh_cv_os_type != no && break
      done
    ])
  case $ksh_cv_os_type in	#((
    AIX)
      AC_DEFINE(_ALL_SOURCE)dnl
      ;;			#(
    Interix)
      AC_DEFINE(_ALL_SOURCE)dnl
      ;;			#(
    ISC)
      AC_DEFINE(OS_ISC)dnl
      ;;			#(
    ISC-posix)
      AC_DEFINE(OS_ISC)dnl
      AC_DEFINE(_POSIX_SOURCE)dnl
      if test "$GCC" = yes; then
	CC="$CC -posix"
      else
	CC="$CC -Xp"
      fi
      ;;			#(
    MINIX)
      AC_DEFINE(_POSIX_SOURCE)dnl
      AC_DEFINE(_POSIX_1_SOURCE, 2)dnl
      AC_DEFINE(_MINIX)dnl
      ;;			#(
    SCO)
      AC_DEFINE(OS_SCO)dnl
      ;;			#(
    TITANOS)
      # Need to use cc -43 to get a shell with job control
      case "$CC" in		#((
        *-43*)  			# Already have -43 option?
	  ;;			#(
	*/cc|*/cc' '|*/cc'	'|cc|cc' '|cc'	') # Using stock compiler?
	  CC="$CC -43"
	  ;;			#(
      esac			#))
      #
      # Force dirent check to find the right thing.  There is a dirent.h
      # (and a sys/dirent.h) file which compiles, but generates garbage...
      #
      ac_cv_header_dirent_dirent_h=no
      ac_cv_header_dirent_sys_ndir_h=no
      ac_cv_header_dirent_sys_dir_h=yes
      ;;			#(
    NEXT)
      #
      # NeXT 3.2 (other versions?) - cc -E doesn't work and /lib/cpp
      # doesn't define things that need defining, so tests that rely
      # on $CPP will break.
      #
      AC_EGREP_CPP([Bad NeXT], [#include <signal.h>
	#if !defined(SIGINT) || !defined(SIGQUIT)
	    Bad NeXT
	#endif
	], AC_MSG_ERROR([
There is a problem on NeXT boxes resulting in a bad siglist.out file being
generated (which breaks the trap and kill commands) and probably resulting
in many configuration tests not working correctly.

You appear to have this problem - see the comments on NeXT in the pdksh
README file for work arounds.]))dnl
      ;;			#(
    HPUX)
      #
      # In some versions of hpux (eg, 10.2), getwd & getcwd will dump core
      # if directory is not readble.
      #
      # name is used in test program
      AC_CACHE_CHECK(for bug in getwd, ksh_cv_hpux_getwd_bug,
	[ tmpdir=conftest.dir
	  if mkdir $tmpdir ; then
	    AC_TRY_RUN([
		int
		main()
		{
		  char buf[8 * 1024];
		  char *dirname = "conftest.dir";
		  int ok = 0;
		  if (chdir(dirname) < 0)
		    exit(2);
		  if (chmod(".", 0) < 0)
		    exit(3);
		  /* Test won't work if run as root - so don't be root */
		  if (getuid() == 0 || geteuid() == 0)
		    setresuid(1, 1, 1);	/* hpux has this */
#ifdef HAVE_GETWD /* silly since HAVE_* tests haven't been done yet */
		  {
		      extern char *getwd();
		      ok = getwd(buf) == 0;
		  }
#else
		  {
		      extern char *getcwd();
		      ok = getcwd(buf, sizeof(buf)) == 0;
		  }
#endif
		  exit(ok ? 0 : 10);
		  return ok ? 0 : 10;
		}],
	       ksh_cv_hpux_getwd_bug=no, ksh_cv_hpux_getwd_bug=yes,
	       AC_MSG_WARN(assuming getwd broken); ksh_cv_hpux_getwd_bug=yes)
	       test -d $tmpdir && rmdir $tmpdir
	  else
	     AC_MSG_ERROR(could not make temp directory for test); ksh_cv_hpux_getwd_bug=yes
	  fi])
      if test $ksh_cv_hpux_getwd_bug = yes; then
	AC_DEFINE(HPUX_GETWD_BUG)
      fi
      ;;			#(
  esac				#))
 ])dnl
dnl
dnl
dnl Some systems (eg, SunOS 4.0.3) have <termios.h> and <termio.h> but don't
dnl have the related functions/defines (eg, tcsetattr(), TCSADRAIN, etc.)
dnl or the functions don't work well with tty process groups.  Sun's bad
dnl termios can be detected by the lack of tcsetattr(), but its bad termio
dnl is harder to detect - so check for (sane) termios first, then check for
dnl BSD, then termio.
AC_DEFUN(KSH_TERM_CHECK,
 [AC_CACHE_CHECK(terminal interface, ksh_cv_term_check,
    [AC_TRY_LINK([#include <termios.h>], [
        struct termios t;
#if defined(ultrix) || defined(__ultrix__)
         Termios in ultrix 4.2 botches type-ahead when going from cooked to
         cbreak mode.  The BSD tty interface works fine though, so use it
         (would be good to know if alter versions of ultrix work).
#endif /* ultrix */
         tcgetattr(0, &t); tcsetattr(0, TCSADRAIN, &t);
      ], ksh_cv_term_check=termios,
	[AC_TRY_LINK([#include <sys/ioctl.h>], [
	    struct sgttyb sb; ioctl(0, TIOCGETP, &sb);
#ifdef TIOCGATC
	    { struct ttychars lc; ioctl(0, TIOCGATC, &lc); }
#else /* TIOCGATC */
	    { struct tchars tc; ioctl(0, TIOCGETC, &tc); }
# ifdef TIOCGLTC
	    { struct ltchars ltc; ioctl(0, TIOCGLTC, &ltc); }
# endif /* TIOCGLTC */
#endif /* TIOCGATC */
	  ], ksh_cv_term_check=bsd,
	    [AC_CHECK_HEADER(termio.h, ksh_cv_term_check=termio,
	      ksh_cv_term_check=sgtty)])])])
  if test $ksh_cv_term_check = termios; then
    AC_DEFINE(HAVE_TERMIOS_H)
dnl Don't know of a system on which this fails...
dnl     AC_CACHE_CHECK(sys/ioctl.h can be included with termios.h,
dnl       ksh_cv_sys_ioctl_with_termios,
dnl       [AC_TRY_COMPILE([#include <termios.h>
dnl #include <sys/ioctl.h>], , ksh_cv_sys_ioctl_with_termios=yes,
dnl 	ksh_cv_sys_ioctl_with_termios=no)])
dnl     if test $ksh_cv_sys_ioctl_with_termios = yes; then
dnl       AC_DEFINE(SYS_IOCTL_WITH_TERMIOS)
dnl     fi
  elif test $ksh_cv_term_check = termio; then
    AC_DEFINE(HAVE_TERMIO_H)
dnl Don't know of a system on which this fails...
dnl     AC_CACHE_CHECK(sys/ioctl.h can be included with termio.h,
dnl       ksh_cv_sys_ioctl_with_termio,
dnl       [AC_TRY_COMPILE([#include <termio.h>
dnl #include <sys/ioctl.h>], , ksh_cv_sys_ioctl_with_termio=yes,
dnl 	ksh_cv_sys_ioctl_with_termio=no)])
dnl     if test $ksh_cv_sys_ioctl_with_termio = yes; then
dnl       AC_DEFINE(SYS_IOCTL_WITH_TERMIO)
dnl     fi
  fi
 ])dnl
dnl
dnl
dnl  Check if lstat() is available - special test needed 'cause lstat only
dnl becomes visable if <sys/stat.h> is included (linux 1.3.x)...
AC_DEFUN(KSH_FUNC_LSTAT,
[AC_CACHE_CHECK(for lstat, ksh_cv_func_lstat,
[AC_TRY_LINK([
#include <sys/types.h>
#include <sys/stat.h>
	], [
	  struct stat statb;
	  lstat("/", &statb);
	],
       ksh_cv_func_lstat=yes, ksh_cv_func_lstat=no)])
if test $ksh_cv_func_lstat = yes; then
  AC_DEFINE(HAVE_LSTAT)
fi
])
dnl
dnl
dnl Like AC_HEADER_SYS_WAIT, only HAVE_SYS_WAIT_H if sys/wait.h exists and
dnl defines POSIX_SYS_WAIT if it is posix compatable.  This way things
dnl like WNOHANG, WUNTRACED can still be used.
AC_DEFUN(KSH_HEADER_SYS_WAIT,
[AC_CACHE_CHECK([for sys/wait.h that is POSIX.1 compatible],
  ksh_cv_header_sys_wait_h,
[AC_MSG_RESULT(further testing...)
AC_HEADER_SYS_WAIT
ksh_cv_header_sys_wait_h=$ac_cv_header_sys_wait_h
unset ac_cv_header_sys_wait_h
AC_MSG_CHECKING(if we got a POSIX.1 compatible sys/wait.h)])
AC_CHECK_HEADERS(sys/wait.h)
if test $ksh_cv_header_sys_wait_h = yes; then
  AC_DEFINE(POSIX_SYS_WAIT)dnl
fi
])
