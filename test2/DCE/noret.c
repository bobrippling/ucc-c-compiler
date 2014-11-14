// ensure linkage:
// RUN: %ucc -g -o %t %s

_Noreturn void abort(void);

main()
{
	int local = 5;

	abort();

	int local2 = 3;

	g(); // no undefined ref - DCE
}
