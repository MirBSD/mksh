/*	$OpenBSD: jobs.c,v 1.36 2007/09/06 19:57:47 otto Exp $	*/

#include "sh.h"

__RCSID("$MirOS: src/bin/mksh/jobs.c,v 1.40.2.1 2008/11/22 13:20:31 tg Exp $");

/* Order important! */
#define PRUNNING	0
#define PEXITED		1
#define PSIGNALLED	2
#define PSTOPPED	3

typedef struct proc	Proc;
struct proc {
	Proc *next;		/* next process in pipeline (if any) */
	int state;
	int status;		/* wait status */
	pid_t pid;		/* process id */
	char command[48];	/* process command string */
};

/* Notify/print flag - j_print() argument */
#define JP_NONE		0	/* don't print anything */
#define JP_SHORT	1	/* print signals processes were killed by */
#define JP_MEDIUM	2	/* print [job-num] -/+ command */
#define JP_LONG		3	/* print [job-num] -/+ pid command */
#define JP_PGRP		4	/* print pgrp */

/* put_job() flags */
#define PJ_ON_FRONT	0	/* at very front */
#define PJ_PAST_STOPPED	1	/* just past any stopped jobs */

/* Job.flags values */
#define JF_STARTED	0x001	/* set when all processes in job are started */
#define JF_WAITING	0x002	/* set if j_waitj() is waiting on job */
#define JF_W_ASYNCNOTIFY 0x004	/* set if waiting and async notification ok */
#define JF_XXCOM	0x008	/* set for $(command) jobs */
#define JF_FG		0x010	/* running in foreground (also has tty pgrp) */
#define JF_SAVEDTTY	0x020	/* j->ttystate is valid */
#define JF_CHANGED	0x040	/* process has changed state */
#define JF_KNOWN	0x080	/* $! referenced */
#define JF_ZOMBIE	0x100	/* known, unwaited process */
#define JF_REMOVE	0x200	/* flagged for removal (j_jobs()/j_noityf()) */
#define JF_USETTYMODE	0x400	/* tty mode saved if process exits normally */
#define JF_SAVEDTTYPGRP	0x800	/* j->saved_ttypgrp is valid */

typedef struct job Job;
struct job {
	Job *next;		/* next job in list */
	int job;		/* job number: %n */
	int flags;		/* see JF_* */
	volatile int state;	/* job state */
	int status;		/* exit status of last process */
	pid_t pgrp;		/* process group of job */
	pid_t ppid;		/* pid of process that forked job */
	int32_t	age;		/* number of jobs started */
	struct timeval systime;	/* system time used by job */
	struct timeval usrtime;	/* user time used by job */
	Proc *proc_list;	/* process list */
	Proc *last_proc;	/* last process in list */
	Coproc_id coproc_id;	/* 0 or id of coprocess output pipe */
	struct termios ttystate;/* saved tty state for stopped jobs */
	pid_t saved_ttypgrp;	/* saved tty process group for stopped jobs */
};

/* Flags for j_waitj() */
#define JW_NONE		0x00
#define JW_INTERRUPT	0x01	/* ^C will stop the wait */
#define JW_ASYNCNOTIFY	0x02	/* asynchronous notification during wait ok */
#define JW_STOPPEDWAIT	0x04	/* wait even if job stopped */

/* Error codes for j_lookup() */
#define JL_OK		0
#define JL_NOSUCH	1	/* no such job */
#define JL_AMBIG	2	/* %foo or %?foo is ambiguous */
#define JL_INVALID	3	/* non-pid, non-% job id */

static const char *const lookup_msgs[] = {
	null,
	"no such job",
	"ambiguous",
	"argument must be %job or process id",
	NULL
};

static Job *job_list;		/* job list */
static Job *last_job;
static Job *async_job;
static pid_t async_pid;

static int nzombie;		/* # of zombies owned by this process */
static int32_t njobs;		/* # of jobs started */

#ifndef CHILD_MAX
#define CHILD_MAX	_POSIX_CHILD_MAX
#endif

/* held_sigchld is set if sigchld occurs before a job is completely started */
static volatile sig_atomic_t held_sigchld;

static struct shf	*shl_j;
static bool		ttypgrp_ok;	/* set if can use tty pgrps */
static pid_t		restore_ttypgrp = -1;
static int const	tt_sigs[] = { SIGTSTP, SIGTTIN, SIGTTOU };

static void		j_set_async(Job *);
static void		j_startjob(Job *);
static int		j_waitj(Job *, int, const char *);
static void		j_sigchld(int);
static void		j_print(Job *, int, struct shf *);
static Job		*j_lookup(const char *, int *);
static Job		*new_job(void);
static Proc		*new_proc(void);
static void		check_job(Job *);
static void		put_job(Job *, int);
static void		remove_job(Job *, const char *);
static int		kill_job(Job *, int);

/* initialise job control */
void
j_init(int mflagset)
{
	(void)sigemptyset(&sm_default);
	sigprocmask(SIG_SETMASK, &sm_default, NULL);

	(void)sigemptyset(&sm_sigchld);
	sigaddset(&sm_sigchld, SIGCHLD);

	setsig(&sigtraps[SIGCHLD], j_sigchld,
	    SS_RESTORE_ORIG|SS_FORCE|SS_SHTRAP);

	if (!mflagset && Flag(FTALKING))
		Flag(FMONITOR) = 1;

	/* shl_j is used to do asynchronous notification (used in
	 * an interrupt handler, so need a distinct shf)
	 */
	shl_j = shf_fdopen(2, SHF_WR, NULL);

	if (Flag(FMONITOR) || Flag(FTALKING)) {
		int i;

		/* the TF_SHELL_USES test is a kludge that lets us know if
		 * if the signals have been changed by the shell.
		 */
		for (i = NELEM(tt_sigs); --i >= 0; ) {
			sigtraps[tt_sigs[i]].flags |= TF_SHELL_USES;
			/* j_change() sets this to SS_RESTORE_DFL if FMONITOR */
			setsig(&sigtraps[tt_sigs[i]], SIG_IGN,
			    SS_RESTORE_IGN|SS_FORCE);
		}
	}

	/* j_change() calls tty_init() */
	if (Flag(FMONITOR))
		j_change();
	else if (Flag(FTALKING))
		tty_init(true);
}

/* job cleanup before shell exit */
void
j_exit(void)
{
	/* kill stopped, and possibly running, jobs */
	Job	*j;
	int	killed = 0;

	for (j = job_list; j != NULL; j = j->next) {
		if (j->ppid == procpid &&
		    (j->state == PSTOPPED ||
		    (j->state == PRUNNING &&
		    ((j->flags & JF_FG) ||
		    (Flag(FLOGIN) && !Flag(FNOHUP) && procpid == kshpid))))) {
			killed = 1;
			if (j->pgrp == 0)
				kill_job(j, SIGHUP);
			else
				killpg(j->pgrp, SIGHUP);
			if (j->state == PSTOPPED) {
				if (j->pgrp == 0)
					kill_job(j, SIGCONT);
				else
					killpg(j->pgrp, SIGCONT);
			}
		}
	}
	if (killed)
		sleep(1);
	j_notify();

	if (kshpid == procpid && restore_ttypgrp >= 0) {
		/* Need to restore the tty pgrp to what it was when the
		 * shell started up, so that the process that started us
		 * will be able to access the tty when we are done.
		 * Also need to restore our process group in case we are
		 * about to do an exec so that both our parent and the
		 * process we are to become will be able to access the tty.
		 */
		tcsetpgrp(tty_fd, restore_ttypgrp);
		setpgid(0, restore_ttypgrp);
	}
	if (Flag(FMONITOR)) {
		Flag(FMONITOR) = 0;
		j_change();
	}
}

/* turn job control on or off according to Flag(FMONITOR) */
void
j_change(void)
{
	int i;

	if (Flag(FMONITOR)) {
		bool use_tty = Flag(FTALKING);

		/* Don't call tcgetattr() 'til we own the tty process group */
		if (use_tty)
			tty_init(false);

		/* no controlling tty, no SIGT* */
		if ((ttypgrp_ok = use_tty && tty_fd >= 0 && tty_devtty)) {
			setsig(&sigtraps[SIGTTIN], SIG_DFL,
			    SS_RESTORE_ORIG|SS_FORCE);
			/* wait to be given tty (POSIX.1, B.2, job control) */
			while (1) {
				pid_t ttypgrp;

				if ((ttypgrp = tcgetpgrp(tty_fd)) < 0) {
					warningf(false,
					    "j_init: tcgetpgrp() failed: %s",
					    strerror(errno));
					ttypgrp_ok = false;
					break;
				}
				if (ttypgrp == kshpgrp)
					break;
				kill(0, SIGTTIN);
			}
		}
		for (i = NELEM(tt_sigs); --i >= 0; )
			setsig(&sigtraps[tt_sigs[i]], SIG_IGN,
			    SS_RESTORE_DFL|SS_FORCE);
		if (ttypgrp_ok && kshpgrp != kshpid) {
			if (setpgid(0, kshpid) < 0) {
				warningf(false,
				    "j_init: setpgid() failed: %s",
				    strerror(errno));
				ttypgrp_ok = false;
			} else {
				if (tcsetpgrp(tty_fd, kshpid) < 0) {
					warningf(false,
					    "j_init: tcsetpgrp() failed: %s",
					    strerror(errno));
					ttypgrp_ok = false;
				} else
					restore_ttypgrp = kshpgrp;
				kshpgrp = kshpid;
			}
		}
		if (use_tty && !ttypgrp_ok)
			warningf(false, "warning: won't have full job control");
		if (tty_fd >= 0)
			tcgetattr(tty_fd, &tty_state);
	} else {
		ttypgrp_ok = false;
		if (Flag(FTALKING))
			for (i = NELEM(tt_sigs); --i >= 0; )
				setsig(&sigtraps[tt_sigs[i]], SIG_IGN,
				    SS_RESTORE_IGN|SS_FORCE);
		else
			for (i = NELEM(tt_sigs); --i >= 0; ) {
				if (sigtraps[tt_sigs[i]].flags &
				    (TF_ORIG_IGN | TF_ORIG_DFL))
					setsig(&sigtraps[tt_sigs[i]],
					    (sigtraps[tt_sigs[i]].flags & TF_ORIG_IGN) ?
					    SIG_IGN : SIG_DFL,
					    SS_RESTORE_ORIG|SS_FORCE);
			}
		if (!Flag(FTALKING))
			tty_close();
	}
}

/* execute tree in child subprocess */
int
exchild(struct op *t, int flags, /* used if XPCLOSE or XCCLOSE */ int close_fd)
{
	static Proc	*last_proc;	/* for pipelines */

	int		i;
	sigset_t	omask;
	Proc		*p;
	Job		*j;
	int		rv = 0;
	int		forksleep;
	int		ischild;

	if (flags & XEXEC)
		/* Clear XFORK|XPCLOSE|XCCLOSE|XCOPROC|XPIPEO|XPIPEI|XXCOM|XBGND
		 * (also done in another execute() below)
		 */
		return execute(t, flags & (XEXEC | XERROK));

	/* no SIGCHLDs while messing with job and process lists */
	sigprocmask(SIG_BLOCK, &sm_sigchld, &omask);

	p = new_proc();
	p->next = NULL;
	p->state = PRUNNING;
	p->status = 0;
	p->pid = 0;

	/* link process into jobs list */
	if (flags & XPIPEI) {	/* continuing with a pipe */
		if (!last_job)
			internal_errorf(
			    "exchild: XPIPEI and no last_job - pid %d",
			    (int)procpid);
		j = last_job;
		if (last_proc)
			last_proc->next = p;
		last_proc = p;
	} else {
		j = new_job(); /* fills in j->job */
		/* we don't consider XXCOMs foreground since they don't get
		 * tty process group and we don't save or restore tty modes.
		 */
		j->flags = (flags & XXCOM) ? JF_XXCOM :
		    ((flags & XBGND) ? 0 : (JF_FG|JF_USETTYMODE));
		timerclear(&j->usrtime);
		timerclear(&j->systime);
		j->state = PRUNNING;
		j->pgrp = 0;
		j->ppid = procpid;
		j->age = ++njobs;
		j->proc_list = p;
		j->coproc_id = 0;
		last_job = j;
		last_proc = p;
		put_job(j, PJ_PAST_STOPPED);
	}

	snptreef(p->command, sizeof(p->command), "%T", t);

	/* create child process */
	forksleep = 1;
	while ((i = fork()) < 0 && errno == EAGAIN && forksleep < 32) {
		if (intrsig)	 /* allow user to ^C out... */
			break;
		sleep(forksleep);
		forksleep <<= 1;
	}
	if (i < 0) {
		kill_job(j, SIGKILL);
		remove_job(j, "fork failed");
		sigprocmask(SIG_SETMASK, &omask, NULL);
		errorf("cannot fork - try again");
	}
	ischild = i == 0;
	if (ischild)
		p->pid = procpid = getpid();
	else
		p->pid = i;

	/* Ensure next child gets a (slightly) different $RANDOM sequence */
	change_random(((unsigned long)p->pid << 1) | (ischild ? 1 : 0));

	/* job control set up */
	if (Flag(FMONITOR) && !(flags&XXCOM)) {
		int	dotty = 0;
		if (j->pgrp == 0) {	/* First process */
			j->pgrp = p->pid;
			dotty = 1;
		}

		/* set pgrp in both parent and child to deal with race
		 * condition
		 */
		setpgid(p->pid, j->pgrp);
		if (ttypgrp_ok && dotty && !(flags & XBGND))
			tcsetpgrp(tty_fd, j->pgrp);
	}

	/* used to close pipe input fd */
	if (close_fd >= 0 && (((flags & XPCLOSE) && !ischild) ||
	    ((flags & XCCLOSE) && ischild)))
		close(close_fd);
	if (ischild) {		/* child */
		/* Do this before restoring signal */
		if (flags & XCOPROC)
			coproc_cleanup(false);
		sigprocmask(SIG_SETMASK, &omask, NULL);
		cleanup_parents_env();
		/* If FMONITOR or FTALKING is set, these signals are ignored,
		 * if neither FMONITOR nor FTALKING are set, the signals have
		 * their inherited values.
		 */
		if (Flag(FMONITOR) && !(flags & XXCOM)) {
			for (i = NELEM(tt_sigs); --i >= 0; )
				setsig(&sigtraps[tt_sigs[i]], SIG_DFL,
				    SS_RESTORE_DFL|SS_FORCE);
		}
#if HAVE_NICE
		if (Flag(FBGNICE) && (flags & XBGND))
			(void)nice(4);
#endif
		if ((flags & XBGND) && !Flag(FMONITOR)) {
			setsig(&sigtraps[SIGINT], SIG_IGN,
			    SS_RESTORE_IGN|SS_FORCE);
			setsig(&sigtraps[SIGQUIT], SIG_IGN,
			    SS_RESTORE_IGN|SS_FORCE);
			if ((!(flags & (XPIPEI | XCOPROC))) &&
			    ((i = open("/dev/null", 0)) > 0)) {
				(void)ksh_dup2(i, 0, true);
				close(i);
			}
		}
		remove_job(j, "child");	/* in case of $(jobs) command */
		nzombie = 0;
		ttypgrp_ok = false;
		Flag(FMONITOR) = 0;
		Flag(FTALKING) = 0;
		tty_close();
		cleartraps();
		execute(t, (flags & XERROK) | XEXEC); /* no return */
#ifndef MKSH_SMALL
		if (t->type == TPIPE)
			unwind(LLEAVE);
		internal_warningf("exchild: execute() returned");
		fptreef(shl_out, 2, "exchild: tried to execute {\n%T\n}\n", t);
		shf_flush(shl_out);
#endif
		unwind(LLEAVE);
		/* NOTREACHED */
	}

	/* shell (parent) stuff */
	if (!(flags & XPIPEO)) {	/* last process in a job */
		j_startjob(j);
		if (flags & XCOPROC) {
			j->coproc_id = coproc.id;
			coproc.njobs++; /* n jobs using co-process output */
			coproc.job = (void *) j; /* j using co-process input */
		}
		if (flags & XBGND) {
			j_set_async(j);
			if (Flag(FTALKING)) {
				shf_fprintf(shl_out, "[%d]", j->job);
				for (p = j->proc_list; p; p = p->next)
					shf_fprintf(shl_out, " %d",
					    (int)p->pid);
				shf_putchar('\n', shl_out);
				shf_flush(shl_out);
			}
		} else
			rv = j_waitj(j, JW_NONE, "jw:last proc");
	}

	sigprocmask(SIG_SETMASK, &omask, NULL);

	return rv;
}

/* start the last job: only used for $(command) jobs */
void
startlast(void)
{
	sigset_t omask;

	sigprocmask(SIG_BLOCK, &sm_sigchld, &omask);

	if (last_job) { /* no need to report error - waitlast() will do it */
		/* ensure it isn't removed by check_job() */
		last_job->flags |= JF_WAITING;
		j_startjob(last_job);
	}
	sigprocmask(SIG_SETMASK, &omask, NULL);
}

/* wait for last job: only used for $(command) jobs */
int
waitlast(void)
{
	int	rv;
	Job	*j;
	sigset_t omask;

	sigprocmask(SIG_BLOCK, &sm_sigchld, &omask);

	j = last_job;
	if (!j || !(j->flags & JF_STARTED)) {
		if (!j)
			warningf(true, "waitlast: no last job");
		else
			internal_warningf("waitlast: not started");
		sigprocmask(SIG_SETMASK, &omask, NULL);
		return 125; /* not so arbitrary, non-zero value */
	}

	rv = j_waitj(j, JW_NONE, "jw:waitlast");

	sigprocmask(SIG_SETMASK, &omask, NULL);

	return rv;
}

/* wait for child, interruptable. */
int
waitfor(const char *cp, int *sigp)
{
	int	rv;
	Job	*j;
	int	ecode;
	int	flags = JW_INTERRUPT|JW_ASYNCNOTIFY;
	sigset_t omask;

	sigprocmask(SIG_BLOCK, &sm_sigchld, &omask);

	*sigp = 0;

	if (cp == NULL) {
		/* wait for an unspecified job - always returns 0, so
		 * don't have to worry about exited/signaled jobs
		 */
		for (j = job_list; j; j = j->next)
			/* at&t ksh will wait for stopped jobs - we don't */
			if (j->ppid == procpid && j->state == PRUNNING)
				break;
		if (!j) {
			sigprocmask(SIG_SETMASK, &omask, NULL);
			return -1;
		}
	} else if ((j = j_lookup(cp, &ecode))) {
		/* don't report normal job completion */
		flags &= ~JW_ASYNCNOTIFY;
		if (j->ppid != procpid) {
			sigprocmask(SIG_SETMASK, &omask, NULL);
			return -1;
		}
	} else {
		sigprocmask(SIG_SETMASK, &omask, NULL);
		if (ecode != JL_NOSUCH)
			bi_errorf("%s: %s", cp, lookup_msgs[ecode]);
		return -1;
	}

	/* at&t ksh will wait for stopped jobs - we don't */
	rv = j_waitj(j, flags, "jw:waitfor");

	sigprocmask(SIG_SETMASK, &omask, NULL);

	if (rv < 0) /* we were interrupted */
		*sigp = 128 + -rv;

	return rv;
}

/* kill (built-in) a job */
int
j_kill(const char *cp, int sig)
{
	Job	*j;
	int	rv = 0;
	int	ecode;
	sigset_t omask;

	sigprocmask(SIG_BLOCK, &sm_sigchld, &omask);

	if ((j = j_lookup(cp, &ecode)) == NULL) {
		sigprocmask(SIG_SETMASK, &omask, NULL);
		bi_errorf("%s: %s", cp, lookup_msgs[ecode]);
		return 1;
	}

	if (j->pgrp == 0) {	/* started when !Flag(FMONITOR) */
		if (kill_job(j, sig) < 0) {
			bi_errorf("%s: %s", cp, strerror(errno));
			rv = 1;
		}
	} else {
		if (j->state == PSTOPPED && (sig == SIGTERM || sig == SIGHUP))
			(void) killpg(j->pgrp, SIGCONT);
		if (killpg(j->pgrp, sig) < 0) {
			bi_errorf("%s: %s", cp, strerror(errno));
			rv = 1;
		}
	}

	sigprocmask(SIG_SETMASK, &omask, NULL);

	return rv;
}

/* fg and bg built-ins: called only if Flag(FMONITOR) set */
int
j_resume(const char *cp, int bg)
{
	Job	*j;
	Proc	*p;
	int	ecode;
	int	running;
	int	rv = 0;
	sigset_t omask;

	sigprocmask(SIG_BLOCK, &sm_sigchld, &omask);

	if ((j = j_lookup(cp, &ecode)) == NULL) {
		sigprocmask(SIG_SETMASK, &omask, NULL);
		bi_errorf("%s: %s", cp, lookup_msgs[ecode]);
		return 1;
	}

	if (j->pgrp == 0) {
		sigprocmask(SIG_SETMASK, &omask, NULL);
		bi_errorf("job not job-controlled");
		return 1;
	}

	if (bg)
		shprintf("[%d] ", j->job);

	running = 0;
	for (p = j->proc_list; p != NULL; p = p->next) {
		if (p->state == PSTOPPED) {
			p->state = PRUNNING;
			p->status = 0;
			running = 1;
		}
		shf_puts(p->command, shl_stdout);
		if (p->next)
			shf_puts("| ", shl_stdout);
	}
	shf_putc('\n', shl_stdout);
	shf_flush(shl_stdout);
	if (running)
		j->state = PRUNNING;

	put_job(j, PJ_PAST_STOPPED);
	if (bg)
		j_set_async(j);
	else {
		/* attach tty to job */
		if (j->state == PRUNNING) {
			if (ttypgrp_ok && (j->flags & JF_SAVEDTTY))
				tcsetattr(tty_fd, TCSADRAIN, &j->ttystate);
			/* See comment in j_waitj regarding saved_ttypgrp. */
			if (ttypgrp_ok &&
			    tcsetpgrp(tty_fd, (j->flags & JF_SAVEDTTYPGRP) ?
			    j->saved_ttypgrp : j->pgrp) < 0) {
				rv = errno;
				if (j->flags & JF_SAVEDTTY)
					tcsetattr(tty_fd, TCSADRAIN, &tty_state);
				sigprocmask(SIG_SETMASK, &omask,
				    NULL);
				bi_errorf("1st tcsetpgrp(%d, %d) failed: %s",
				    tty_fd,
				    (int) ((j->flags & JF_SAVEDTTYPGRP) ?
				    j->saved_ttypgrp : j->pgrp),
				    strerror(rv));
				return 1;
			}
		}
		j->flags |= JF_FG;
		j->flags &= ~JF_KNOWN;
		if (j == async_job)
			async_job = NULL;
	}

	if (j->state == PRUNNING && killpg(j->pgrp, SIGCONT) < 0) {
		int err = errno;

		if (!bg) {
			j->flags &= ~JF_FG;
			if (ttypgrp_ok && (j->flags & JF_SAVEDTTY))
				tcsetattr(tty_fd, TCSADRAIN, &tty_state);
			if (ttypgrp_ok && tcsetpgrp(tty_fd, kshpgrp) < 0)
				warningf(true,
				    "fg: 2nd tcsetpgrp(%d, %ld) failed: %s",
				    tty_fd, (long)kshpgrp, strerror(errno));
		}
		sigprocmask(SIG_SETMASK, &omask, NULL);
		bi_errorf("cannot continue job %s: %s",
		    cp, strerror(err));
		return 1;
	}
	if (!bg) {
		if (ttypgrp_ok) {
			j->flags &= ~(JF_SAVEDTTY | JF_SAVEDTTYPGRP);
		}
		rv = j_waitj(j, JW_NONE, "jw:resume");
	}
	sigprocmask(SIG_SETMASK, &omask, NULL);
	return rv;
}

/* are there any running or stopped jobs ? */
int
j_stopped_running(void)
{
	Job	*j;
	int	which = 0;

	for (j = job_list; j != NULL; j = j->next) {
		if (j->ppid == procpid && j->state == PSTOPPED)
			which |= 1;
		if (Flag(FLOGIN) && !Flag(FNOHUP) && procpid == kshpid &&
		    j->ppid == procpid && j->state == PRUNNING)
			which |= 2;
	}
	if (which) {
		shellf("You have %s%s%s jobs\n",
		    which & 1 ? "stopped" : "",
		    which == 3 ? " and " : "",
		    which & 2 ? "running" : "");
		return 1;
	}

	return 0;
}

int
j_njobs(void)
{
	Job *j;
	int nj = 0;
	sigset_t omask;

	sigprocmask(SIG_BLOCK, &sm_sigchld, &omask);
	for (j = job_list; j; j = j->next)
		nj++;

	sigprocmask(SIG_SETMASK, &omask, NULL);
	return nj;
}


/* list jobs for jobs built-in */
int
j_jobs(const char *cp, int slp,
    int nflag)		/* 0: short, 1: long, 2: pgrp */
{
	Job	*j, *tmp;
	int	how;
	int	zflag = 0;
	sigset_t omask;

	sigprocmask(SIG_BLOCK, &sm_sigchld, &omask);

	if (nflag < 0) { /* kludge: print zombies */
		nflag = 0;
		zflag = 1;
	}
	if (cp) {
		int	ecode;

		if ((j = j_lookup(cp, &ecode)) == NULL) {
			sigprocmask(SIG_SETMASK, &omask, NULL);
			bi_errorf("%s: %s", cp, lookup_msgs[ecode]);
			return 1;
		}
	} else
		j = job_list;
	how = slp == 0 ? JP_MEDIUM : (slp == 1 ? JP_LONG : JP_PGRP);
	for (; j; j = j->next) {
		if ((!(j->flags & JF_ZOMBIE) || zflag) &&
		    (!nflag || (j->flags & JF_CHANGED))) {
			j_print(j, how, shl_stdout);
			if (j->state == PEXITED || j->state == PSIGNALLED)
				j->flags |= JF_REMOVE;
		}
		if (cp)
			break;
	}
	/* Remove jobs after printing so there won't be multiple + or - jobs */
	for (j = job_list; j; j = tmp) {
		tmp = j->next;
		if (j->flags & JF_REMOVE)
			remove_job(j, "jobs");
	}
	sigprocmask(SIG_SETMASK, &omask, NULL);
	return 0;
}

/* list jobs for top-level notification */
void
j_notify(void)
{
	Job	*j, *tmp;
	sigset_t omask;

	sigprocmask(SIG_BLOCK, &sm_sigchld, &omask);
	for (j = job_list; j; j = j->next) {
		if (Flag(FMONITOR) && (j->flags & JF_CHANGED))
			j_print(j, JP_MEDIUM, shl_out);
		/* Remove job after doing reports so there aren't
		 * multiple +/- jobs.
		 */
		if (j->state == PEXITED || j->state == PSIGNALLED)
			j->flags |= JF_REMOVE;
	}
	for (j = job_list; j; j = tmp) {
		tmp = j->next;
		if (j->flags & JF_REMOVE)
			remove_job(j, "notify");
	}
	shf_flush(shl_out);
	sigprocmask(SIG_SETMASK, &omask, NULL);
}

/* Return pid of last process in last asynchronous job */
pid_t
j_async(void)
{
	sigset_t omask;

	sigprocmask(SIG_BLOCK, &sm_sigchld, &omask);

	if (async_job)
		async_job->flags |= JF_KNOWN;

	sigprocmask(SIG_SETMASK, &omask, NULL);

	return async_pid;
}

/* Make j the last async process
 *
 * If jobs are compiled in then this routine expects sigchld to be blocked.
 */
static void
j_set_async(Job *j)
{
	Job	*jl, *oldest;

	if (async_job && (async_job->flags & (JF_KNOWN|JF_ZOMBIE)) == JF_ZOMBIE)
		remove_job(async_job, "async");
	if (!(j->flags & JF_STARTED)) {
		internal_warningf("j_async: job not started");
		return;
	}
	async_job = j;
	async_pid = j->last_proc->pid;
	while (nzombie > CHILD_MAX) {
		oldest = NULL;
		for (jl = job_list; jl; jl = jl->next)
			if (jl != async_job && (jl->flags & JF_ZOMBIE) &&
			    (!oldest || jl->age < oldest->age))
				oldest = jl;
		if (!oldest) {
			/* XXX debugging */
			if (!(async_job->flags & JF_ZOMBIE) || nzombie != 1) {
				internal_warningf("j_async: bad nzombie (%d)",
				    nzombie);
				nzombie = 0;
			}
			break;
		}
		remove_job(oldest, "zombie");
	}
}

/* Start a job: set STARTED, check for held signals and set j->last_proc
 *
 * If jobs are compiled in then this routine expects sigchld to be blocked.
 */
static void
j_startjob(Job *j)
{
	Proc	*p;

	j->flags |= JF_STARTED;
	for (p = j->proc_list; p->next; p = p->next)
		;
	j->last_proc = p;

	if (held_sigchld) {
		held_sigchld = 0;
		/* Don't call j_sigchld() as it may remove job... */
		kill(procpid, SIGCHLD);
	}
}

/*
 * wait for job to complete or change state
 *
 * If jobs are compiled in then this routine expects sigchld to be blocked.
 */
static int
j_waitj(Job *j,
    int flags,			/* see JW_* */
    const char *where)
{
	int	rv;

	/*
	 * No auto-notify on the job we are waiting on.
	 */
	j->flags |= JF_WAITING;
	if (flags & JW_ASYNCNOTIFY)
		j->flags |= JF_W_ASYNCNOTIFY;

	if (!Flag(FMONITOR))
		flags |= JW_STOPPEDWAIT;

	while (j->state == PRUNNING ||
	    ((flags & JW_STOPPEDWAIT) && j->state == PSTOPPED)) {
		sigsuspend(&sm_default);
		if (fatal_trap) {
			int oldf = j->flags & (JF_WAITING|JF_W_ASYNCNOTIFY);
			j->flags &= ~(JF_WAITING|JF_W_ASYNCNOTIFY);
			runtraps(TF_FATAL);
			j->flags |= oldf; /* not reached... */
		}
		if ((flags & JW_INTERRUPT) && (rv = trap_pending())) {
			j->flags &= ~(JF_WAITING|JF_W_ASYNCNOTIFY);
			return -rv;
		}
	}
	j->flags &= ~(JF_WAITING|JF_W_ASYNCNOTIFY);

	if (j->flags & JF_FG) {
		int	status;

		j->flags &= ~JF_FG;
		if (Flag(FMONITOR) && ttypgrp_ok && j->pgrp) {
			/*
			 * Save the tty's current pgrp so it can be restored
			 * when the job is foregrounded.  This is to
			 * deal with things like the GNU su which does
			 * a fork/exec instead of an exec (the fork means
			 * the execed shell gets a different pid from its
			 * pgrp, so naturally it sets its pgrp and gets hosed
			 * when it gets foregrounded by the parent shell which
			 * has restored the tty's pgrp to that of the su
			 * process).
			 */
			if (j->state == PSTOPPED &&
			    (j->saved_ttypgrp = tcgetpgrp(tty_fd)) >= 0)
				j->flags |= JF_SAVEDTTYPGRP;
			if (tcsetpgrp(tty_fd, kshpgrp) < 0)
				warningf(true,
				    "j_waitj: tcsetpgrp(%d, %ld) failed: %s",
				    tty_fd, (long)kshpgrp, strerror(errno));
			if (j->state == PSTOPPED) {
				j->flags |= JF_SAVEDTTY;
				tcgetattr(tty_fd, &j->ttystate);
			}
		}
		if (tty_fd >= 0) {
			/* Only restore tty settings if job was originally
			 * started in the foreground.  Problems can be
			 * caused by things like 'more foobar &' which will
			 * typically get and save the shell's vi/emacs tty
			 * settings before setting up the tty for itself;
			 * when more exits, it restores the 'original'
			 * settings, and things go down hill from there...
			 */
			if (j->state == PEXITED && j->status == 0 &&
			    (j->flags & JF_USETTYMODE)) {
				tcgetattr(tty_fd, &tty_state);
			} else {
				tcsetattr(tty_fd, TCSADRAIN, &tty_state);
				/* Don't use tty mode if job is stopped and
				 * later restarted and exits.  Consider
				 * the sequence:
				 *	vi foo (stopped)
				 *	...
				 *	stty something
				 *	...
				 *	fg (vi; ZZ)
				 * mode should be that of the stty, not what
				 * was before the vi started.
				 */
				if (j->state == PSTOPPED)
					j->flags &= ~JF_USETTYMODE;
			}
		}
		/* If it looks like user hit ^C to kill a job, pretend we got
		 * one too to break out of for loops, etc.  (at&t ksh does this
		 * even when not monitoring, but this doesn't make sense since
		 * a tty generated ^C goes to the whole process group)
		 */
		status = j->last_proc->status;
		if (Flag(FMONITOR) && j->state == PSIGNALLED &&
		    WIFSIGNALED(status) &&
		    (sigtraps[WTERMSIG(status)].flags & TF_TTY_INTR))
			trapsig(WTERMSIG(status));
	}

	j_usrtime = j->usrtime;
	j_systime = j->systime;
	rv = j->status;

	if (!(flags & JW_ASYNCNOTIFY) &&
	    (!Flag(FMONITOR) || j->state != PSTOPPED)) {
		j_print(j, JP_SHORT, shl_out);
		shf_flush(shl_out);
	}
	if (j->state != PSTOPPED &&
	    (!Flag(FMONITOR) || !(flags & JW_ASYNCNOTIFY)))
		remove_job(j, where);

	return rv;
}

/* SIGCHLD handler to reap children and update job states
 *
 * If jobs are compiled in then this routine expects sigchld to be blocked.
 */
/* ARGSUSED */
static void
j_sigchld(int sig __unused)
{
	int		errno_ = errno;
	Job		*j;
	Proc		*p = NULL;
	int		pid;
	int		status;
	struct rusage	ru0, ru1;

	/* Don't wait for any processes if a job is partially started.
	 * This is so we don't do away with the process group leader
	 * before all the processes in a pipe line are started (so the
	 * setpgid() won't fail)
	 */
	for (j = job_list; j; j = j->next)
		if (j->ppid == procpid && !(j->flags & JF_STARTED)) {
			held_sigchld = 1;
			return;
		}

	getrusage(RUSAGE_CHILDREN, &ru0);
	do {
		pid = waitpid(-1, &status, (WNOHANG|WUNTRACED));

		if (pid <= 0)	/* return if would block (0) ... */
			break;	/* ... or no children or interrupted (-1) */

		getrusage(RUSAGE_CHILDREN, &ru1);

		/* find job and process structures for this pid */
		for (j = job_list; j != NULL; j = j->next)
			for (p = j->proc_list; p != NULL; p = p->next)
				if (p->pid == pid)
					goto found;
 found:
		if (j == NULL) {
			/* Can occur if process has kids, then execs shell
			warningf(true, "bad process waited for (pid = %d)",
				pid);
			 */
			ru0 = ru1;
			continue;
		}

		timeradd(&j->usrtime, &ru1.ru_utime, &j->usrtime);
		timersub(&j->usrtime, &ru0.ru_utime, &j->usrtime);
		timeradd(&j->systime, &ru1.ru_stime, &j->systime);
		timersub(&j->systime, &ru0.ru_stime, &j->systime);
		ru0 = ru1;
		p->status = status;
		if (WIFSTOPPED(status))
			p->state = PSTOPPED;
		else if (WIFSIGNALED(status))
			p->state = PSIGNALLED;
		else
			p->state = PEXITED;

		check_job(j);	/* check to see if entire job is done */
	}
	while (1);

	errno = errno_;
}

/*
 * Called only when a process in j has exited/stopped (ie, called only
 * from j_sigchld()).  If no processes are running, the job status
 * and state are updated, asynchronous job notification is done and,
 * if unneeded, the job is removed.
 *
 * If jobs are compiled in then this routine expects sigchld to be blocked.
 */
static void
check_job(Job *j)
{
	int	jstate;
	Proc	*p;

	/* XXX debugging (nasty - interrupt routine using shl_out) */
	if (!(j->flags & JF_STARTED)) {
		internal_warningf("check_job: job started (flags 0x%x)",
		    j->flags);
		return;
	}

	jstate = PRUNNING;
	for (p=j->proc_list; p != NULL; p = p->next) {
		if (p->state == PRUNNING)
			return;	/* some processes still running */
		if (p->state > jstate)
			jstate = p->state;
	}
	j->state = jstate;

	switch (j->last_proc->state) {
	case PEXITED:
		j->status = WEXITSTATUS(j->last_proc->status);
		break;
	case PSIGNALLED:
		j->status = 128 + WTERMSIG(j->last_proc->status);
		break;
	default:
		j->status = 0;
		break;
	}

	/* Note when co-process dies: can't be done in j_wait() nor
	 * remove_job() since neither may be called for non-interactive
	 * shells.
	 */
	if (j->state == PEXITED || j->state == PSIGNALLED) {
		/* No need to keep co-process input any more
		 * (at least, this is what ksh93d thinks)
		 */
		if (coproc.job == j) {
			coproc.job = NULL;
			/* XXX would be nice to get the closes out of here
			 * so they aren't done in the signal handler.
			 * Would mean a check in coproc_getfd() to
			 * do "if job == 0 && write >= 0, close write".
			 */
			coproc_write_close(coproc.write);
		}
		/* Do we need to keep the output? */
		if (j->coproc_id && j->coproc_id == coproc.id &&
		    --coproc.njobs == 0)
			coproc_readw_close(coproc.read);
	}

	j->flags |= JF_CHANGED;
	if (Flag(FMONITOR) && !(j->flags & JF_XXCOM)) {
		/* Only put stopped jobs at the front to avoid confusing
		 * the user (don't want finished jobs effecting %+ or %-)
		 */
		if (j->state == PSTOPPED)
			put_job(j, PJ_ON_FRONT);
		if (Flag(FNOTIFY) &&
		    (j->flags & (JF_WAITING|JF_W_ASYNCNOTIFY)) != JF_WAITING) {
			/* Look for the real file descriptor 2 */
			{
				struct env *ep;
				int fd = 2;

				for (ep = e; ep; ep = ep->oenv)
					if (ep->savefd && ep->savefd[2])
						fd = ep->savefd[2];
				shf_reopen(fd, SHF_WR, shl_j);
			}
			/* Can't call j_notify() as it removes jobs.  The job
			 * must stay in the job list as j_waitj() may be
			 * running with this job.
			 */
			j_print(j, JP_MEDIUM, shl_j);
			shf_flush(shl_j);
			if (!(j->flags & JF_WAITING) && j->state != PSTOPPED)
				remove_job(j, "notify");
		}
	}
	if (!Flag(FMONITOR) && !(j->flags & (JF_WAITING|JF_FG)) &&
	    j->state != PSTOPPED) {
		if (j == async_job || (j->flags & JF_KNOWN)) {
			j->flags |= JF_ZOMBIE;
			j->job = -1;
			nzombie++;
		} else
			remove_job(j, "checkjob");
	}
}

/*
 * Print job status in either short, medium or long format.
 *
 * If jobs are compiled in then this routine expects sigchld to be blocked.
 */
static void
j_print(Job *j, int how, struct shf *shf)
{
	Proc	*p;
	int	state;
	int	status;
	int	coredumped;
	char	jobchar = ' ';
	char	buf[64];
	const char *filler;
	int	output = 0;

	if (how == JP_PGRP) {
		/* POSIX doesn't say what to do it there is no process
		 * group leader (ie, !FMONITOR).  We arbitrarily return
		 * last pid (which is what $! returns).
		 */
		shf_fprintf(shf, "%d\n", (int)(j->pgrp ? j->pgrp :
		    (j->last_proc ? j->last_proc->pid : 0)));
		return;
	}
	j->flags &= ~JF_CHANGED;
	filler = j->job > 10 ?  "\n       " : "\n      ";
	if (j == job_list)
		jobchar = '+';
	else if (j == job_list->next)
		jobchar = '-';

	for (p = j->proc_list; p != NULL;) {
		coredumped = 0;
		switch (p->state) {
		case PRUNNING:
			strlcpy(buf, "Running", sizeof buf);
			break;
		case PSTOPPED:
			strlcpy(buf, sigtraps[WSTOPSIG(p->status)].mess,
			    sizeof buf);
			break;
		case PEXITED:
			if (how == JP_SHORT)
				buf[0] = '\0';
			else if (WEXITSTATUS(p->status) == 0)
				strlcpy(buf, "Done", sizeof buf);
			else
				shf_snprintf(buf, sizeof(buf), "Done (%d)",
				    WEXITSTATUS(p->status));
			break;
		case PSIGNALLED:
#ifdef WCOREDUMP
			if (WCOREDUMP(p->status))
				coredumped = 1;
#endif
			/* kludge for not reporting 'normal termination signals'
			 * (ie, SIGINT, SIGPIPE)
			 */
			if (how == JP_SHORT && !coredumped &&
			    (WTERMSIG(p->status) == SIGINT ||
			    WTERMSIG(p->status) == SIGPIPE)) {
				buf[0] = '\0';
			} else
				strlcpy(buf, sigtraps[WTERMSIG(p->status)].mess,
				    sizeof buf);
			break;
		}

		if (how != JP_SHORT) {
			if (p == j->proc_list)
				shf_fprintf(shf, "[%d] %c ", j->job, jobchar);
			else
				shf_fprintf(shf, "%s", filler);
		}

		if (how == JP_LONG)
			shf_fprintf(shf, "%5d ", (int)p->pid);

		if (how == JP_SHORT) {
			if (buf[0]) {
				output = 1;
				shf_fprintf(shf, "%s%s ",
				    buf, coredumped ? " (core dumped)" : null);
			}
		} else {
			output = 1;
			shf_fprintf(shf, "%-20s %s%s%s", buf, p->command,
			    p->next ? "|" : null,
			    coredumped ? " (core dumped)" : null);
		}

		state = p->state;
		status = p->status;
		p = p->next;
		while (p && p->state == state && p->status == status) {
			if (how == JP_LONG)
				shf_fprintf(shf, "%s%5d %-20s %s%s", filler,
				    (int)p->pid, " ", p->command,
				    p->next ? "|" : null);
			else if (how == JP_MEDIUM)
				shf_fprintf(shf, " %s%s", p->command,
				    p->next ? "|" : null);
			p = p->next;
		}
	}
	if (output)
		shf_putc('\n', shf);
}

/* Convert % sequence to job
 *
 * If jobs are compiled in then this routine expects sigchld to be blocked.
 */
static Job *
j_lookup(const char *cp, int *ecodep)
{
	Job		*j, *last_match;
	Proc		*p;
	int		len, job = 0;

	if (ksh_isdigit(*cp)) {
		getn(cp, &job);
		/* Look for last_proc->pid (what $! returns) first... */
		for (j = job_list; j != NULL; j = j->next)
			if (j->last_proc && j->last_proc->pid == job)
				return j;
		/* ...then look for process group (this is non-POSIX,
		 * but should not break anything */
		for (j = job_list; j != NULL; j = j->next)
			if (j->pgrp && j->pgrp == job)
				return j;
		if (ecodep)
			*ecodep = JL_NOSUCH;
		return NULL;
	}
	if (*cp != '%') {
		if (ecodep)
			*ecodep = JL_INVALID;
		return NULL;
	}
	switch (*++cp) {
	case '\0': /* non-standard */
	case '+':
	case '%':
		if (job_list != NULL)
			return job_list;
		break;

	case '-':
		if (job_list != NULL && job_list->next)
			return job_list->next;
		break;

	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
		getn(cp, &job);
		for (j = job_list; j != NULL; j = j->next)
			if (j->job == job)
				return j;
		break;

	case '?':		/* %?string */
		last_match = NULL;
		for (j = job_list; j != NULL; j = j->next)
			for (p = j->proc_list; p != NULL; p = p->next)
				if (strstr(p->command, cp+1) != NULL) {
					if (last_match) {
						if (ecodep)
							*ecodep = JL_AMBIG;
						return NULL;
					}
					last_match = j;
				}
		if (last_match)
			return last_match;
		break;

	default:		/* %string */
		len = strlen(cp);
		last_match = NULL;
		for (j = job_list; j != NULL; j = j->next)
			if (strncmp(cp, j->proc_list->command, len) == 0) {
				if (last_match) {
					if (ecodep)
						*ecodep = JL_AMBIG;
					return NULL;
				}
				last_match = j;
			}
		if (last_match)
			return last_match;
		break;
	}
	if (ecodep)
		*ecodep = JL_NOSUCH;
	return NULL;
}

static Job	*free_jobs;
static Proc	*free_procs;

/* allocate a new job and fill in the job number.
 *
 * If jobs are compiled in then this routine expects sigchld to be blocked.
 */
static Job *
new_job(void)
{
	int	i;
	Job	*newj, *j;

	if (free_jobs != NULL) {
		newj = free_jobs;
		free_jobs = free_jobs->next;
	} else
		newj = galloc(1, sizeof (Job), APERM);

	/* brute force method */
	for (i = 1; ; i++) {
		for (j = job_list; j && j->job != i; j = j->next)
			;
		if (j == NULL)
			break;
	}
	newj->job = i;

	return newj;
}

/* Allocate new process struct
 *
 * If jobs are compiled in then this routine expects sigchld to be blocked.
 */
static Proc *
new_proc(void)
{
	Proc	*p;

	if (free_procs != NULL) {
		p = free_procs;
		free_procs = free_procs->next;
	} else
		p = galloc(1, sizeof (Proc), APERM);

	return p;
}

/* Take job out of job_list and put old structures into free list.
 * Keeps nzombies, last_job and async_job up to date.
 *
 * If jobs are compiled in then this routine expects sigchld to be blocked.
 */
static void
remove_job(Job *j, const char *where)
{
	Proc	*p, *tmp;
	Job	**prev, *curr;

	prev = &job_list;
	curr = *prev;
	for (; curr != NULL && curr != j; prev = &curr->next, curr = *prev)
		;
	if (curr != j) {
		internal_warningf("remove_job: job not found (%s)", where);
		return;
	}
	*prev = curr->next;

	/* free up proc structures */
	for (p = j->proc_list; p != NULL; ) {
		tmp = p;
		p = p->next;
		tmp->next = free_procs;
		free_procs = tmp;
	}

	if ((j->flags & JF_ZOMBIE) && j->ppid == procpid)
		--nzombie;
	j->next = free_jobs;
	free_jobs = j;

	if (j == last_job)
		last_job = NULL;
	if (j == async_job)
		async_job = NULL;
}

/* put j in a particular location (taking it out job_list if it is there
 * already)
 *
 * If jobs are compiled in then this routine expects sigchld to be blocked.
 */
static void
put_job(Job *j, int where)
{
	Job	**prev, *curr;

	/* Remove job from list (if there) */
	prev = &job_list;
	curr = job_list;
	for (; curr && curr != j; prev = &curr->next, curr = *prev)
		;
	if (curr == j)
		*prev = curr->next;

	switch (where) {
	case PJ_ON_FRONT:
		j->next = job_list;
		job_list = j;
		break;

	case PJ_PAST_STOPPED:
		prev = &job_list;
		curr = job_list;
		for (; curr && curr->state == PSTOPPED; prev = &curr->next,
		    curr = *prev)
			;
		j->next = curr;
		*prev = j;
		break;
	}
}

/* nuke a job (called when unable to start full job).
 *
 * If jobs are compiled in then this routine expects sigchld to be blocked.
 */
static int
kill_job(Job *j, int sig)
{
	Proc	*p;
	int	rval = 0;

	for (p = j->proc_list; p != NULL; p = p->next)
		if (p->pid != 0)
			if (kill(p->pid, sig) < 0)
				rval = -1;
	return rval;
}
