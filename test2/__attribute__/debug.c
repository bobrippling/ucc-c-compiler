// RUN: %check %s

__attribute__((__ucc_debug)) int i; // CHECK: warning: debug attribute handled

main()
{
	struct
	{
		__attribute__((__ucc_debug)) char *p; // CHECK: warning: debug attribute handled
	} a;

	a.p = 3;
}
