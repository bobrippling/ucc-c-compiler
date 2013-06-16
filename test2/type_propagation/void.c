// RUN: %ucc -DA %s 2>&1 | grep 'error: a is void'
// RUN: %ucc -DB %s 2>&1 | grep 'error: array of void'

main()
{
#ifdef A
	void a;
#else
	void b[2];
#endif
}
