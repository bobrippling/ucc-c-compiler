// RUN: %ucc -c %s
// RUN: %asmcheck %s
f()
{
	extern f();
	extern (*p)();
	extern d(int (*)());

	d(f);
	d(p);
}
