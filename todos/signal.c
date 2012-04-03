#include "lib/syscalls.h"
#include "lib/sys/signal.h"
#include "lib/stdlib.h"

typedef void (*__sighandler_t) (int);

typedef struct {
#ifdef HAVE_LONGS_AND_STUFF
#  define _SIGSET_NWORDS (1024 / (8 * sizeof (unsigned long int)))
	unsigned long int val[_SIGSET_NWORDS];
#else
#  define _SIGSET_NWORDS (1024 / (8 * sizeof (unsigned int)))
	unsigned int val[2 * _SIGSET_NWORDS];
#endif
} __sigset_t;

typedef struct siginfo siginfo_t; // ehhhhhhhh

struct sigaction
{
	union
	{
		__sighandler_t sa_handler;                       /* if SA_SIGINFO is not set  */
		void (*sa_sigaction) (int, siginfo_t *, void *); /* else */
	} __sigaction_handler;

	__sigset_t sa_mask;

	int sa_flags;

	void (*sa_restorer)(void);
};
#define sa_handler    __sigaction_handler.sa_handler
#define sa_sigaction  __sigaction_handler.sa_sigaction


#define SA_NOCLDSTOP  1 /* Don't send SIGCHLD when children stop.  */
#define SA_NOCLDWAIT  2 /* Don't create zombie on child death.  */
#define SA_SIGINFO    4 /* Invoke signal-catching function with three arguments instead of one.  */

#define SA_ONSTACK   0x08000000 /* Use signal stack by using `sa_restorer'. */
#define SA_RESTART   0x10000000 /* Restart syscall on signal return.  */
#define SA_NODEFER   0x40000000 /* Don't automatically block the signal when its handler is being executed.  */
#define SA_RESETHAND 0x80000000 /* Reset to SIG_DFL on entry to handler.  */
#define SA_INTERRUPT 0x20000000 /* Historical no-op.  */

/* Some aliases for the SA_ constants.  */
#define SA_NOMASK    SA_NODEFER
#define SA_ONESHOT   SA_RESETHAND
#define SA_STACK     SA_ONSTACK

#define SIG_BLOCK     0
#define SIG_UNBLOCK   1
#define SIG_SETMASK   2


int sigaction(int sig, const struct sigaction *act, struct sigaction *oact)
{
	return __syscall(SYS_rt_sigaction, sig, act, oact);
}

sigh()
{
	printf("yo\n");
}

main()
{
	struct sigaction action;

	action.sa_handler = sigh;

	sigaction(SIGABRT, &action, NULL);
	sigaction(SIGTERM, &action, NULL);
}
