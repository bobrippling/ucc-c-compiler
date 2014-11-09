// RUN: %ucc -fsyntax-only %s

f(int *p);

main()
{
	// we shouldn't try to cleanup c_int as a variable
	typedef __attribute((cleanup(f))) int c_int;
}
