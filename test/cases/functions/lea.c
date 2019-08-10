// RUN: %ucc -target x86_64-linux -S -o %t %s -fno-pic
// RUN:   grep 'lea. *_\?f' %t
// RUN:   grep 'mov. *_\?p' %t
// RUN: ! grep 'mov. *_\?f' %t
// RUN: ! grep 'lea. *_\?p' %t

f()
{
	extern f();
	extern (*p)();
	extern d(int (*)());

	d(f);
	d(p);
}
