// RUN: %check %s

f(int x[], int y[2])
{
	int a[2];
	return
		sizeof x + // CHECK: warning: array-argument evaluates to sizeof(int *), not sizeof(int[])
		sizeof y + // CHECK: warning: array-argument evaluates to sizeof(int *), not sizeof(int[2])

		sizeof(int) +
		sizeof a;
}
