// RUN: %check %s

f(int x[])
{
	int a[2];
	return
		sizeof x + // CHECK: warning: array parameter size is sizeof(int *), not sizeof(int[])
		sizeof(int) +
		sizeof a;
}
