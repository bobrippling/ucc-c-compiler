// RUN: %layout_check %s
enum
{
	SIGHUP,
	SIGINT,
	SIGQUIT,
	SIGILL,
	SIGTRAP,
	SIGABRT,
	SIGFPE,
	SIGKILL,
	SIGUSR1,
	SIGSEGV,
	SIGUSR2,
	SIGPIPE,
	SIGALRM,
	SIGTERM,
	SIGCHLD,
	SIGCONT,
	SIGSTOP,
	SIGTSTP,
	SIGTTIN,
	SIGTTOU,
};

struct
{
	const char *nam;
	int msk;
} sigs[] = {
	[0 ... 31 - 1] = { "?", 6 },

	[SIGHUP].nam = "SIG""HUP",
	[SIGINT].nam = "SIG""INT",
	[SIGQUIT].nam = "SIG""QUIT",
	[SIGILL].nam = "SIG""ILL",

	[SIGTRAP].nam = "SIG""TRAP",
	[SIGABRT].nam = "SIG""ABRT",
	[SIGFPE].nam = "SIG""FPE",
	[SIGKILL].nam = "SIG""KILL",

	[SIGUSR1].nam = "SIG""USR1",
	[SIGSEGV].nam = "SIG""SEGV",
	[SIGUSR2].nam = "SIG""USR2",
	[SIGPIPE].nam = "SIG""PIPE",

	[SIGALRM].nam = "SIG""ALRM",
	[SIGTERM].nam = "SIG""TERM",
	[SIGCHLD].nam = "SIG""CHLD",
	[SIGCONT].nam = "SIG""CONT",

	[SIGSTOP].nam = "SIG""STOP",
	[SIGTSTP].nam = "SIG""TSTP",
	[SIGTTIN].nam = "SIG""TTIN",
	[SIGTTOU].nam = "SIG""TTOU",
};
