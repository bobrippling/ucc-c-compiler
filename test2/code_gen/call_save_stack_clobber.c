// RUN: %ocheck 0 %s

struct myproc
{
	int ppid;
	int unam;
	int gnam;
	int pc_cpu;
};

const char *proc_state_str(struct myproc *a)
{
	return "";
}

int max_unam_len, max_gnam_len;

void noop()
{
}

const char *machine_proc_display_line_default(struct myproc *p)
{
	static char buf[64];

	noop(buf, sizeof buf,
			"% 7d %-1s "
			"%-*s %-*s "
			"%3.1f"
			,
			p->ppid, proc_state_str(p),
			max_unam_len, p->unam,
			max_gnam_len, p->gnam,
			p->pc_cpu
			);
	/* previously the last couple of arguments would be stored at the
	 * bottom of the stack, which is where callee-save registers go.
	 * now we record stack-spill arguments and adjust where callee-save
	 * registers go to ensure they don't overlap */

	return 7;
}

a(){ return 'a'; }
b(){ return 'b'; }
c(){ return 'c'; }
d(){ return 'd'; }

verify(int a, int b, int x, int c, int d)
{
	if(a != 'a') abort();
	if(b != 'b') abort();
	if(c != 'c') abort();
	if(d != 'd') abort();
	if(x != 7) abort();
}

main()
{
	// ensure use of callee-save
	verify(a(), b(), machine_proc_display_line_default(&(struct myproc){ 0 }), c(), d());
}
