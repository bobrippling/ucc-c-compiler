// RUN: %ocheck 0 %s
void abort(void) __attribute__((noreturn));

void g(int *p)
{
	*p = 3;
}

void f(int a)
{
	/*
	 * There was a code-gen bug with argument addressing (exposed by commit
	 * 5e7ca35c01f9e72710f93391bee0a88d80768040), where functions that stored
	 * arguments directly (rather than push %reg) would store them as a spilt
	 * reg, meaning it wouldn't be treated correctly for addressing, lvalueness,
	 * etc.
	 */
	g(&a);

	if(a != 3)
		abort();
}

int main()
{
	f(-1);
}
