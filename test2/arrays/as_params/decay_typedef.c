// RUN: %ocheck 0 %s
// RUN: %check %s

typedef int A[10];

f(A a)
{
	return sizeof a; // CHECK: warning: array-argument evaluates to sizeof(int *)
}

main()
{
	if( f((void *)0) != sizeof(void *) )
		abort();

	return 0;
}
