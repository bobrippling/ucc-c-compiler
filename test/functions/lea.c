// RUN: %asmcheck %s -fno-pic
f()
{
	extern f();
	extern (*p)();
	extern d(int (*)());

	d(f);
	d(p);
}
