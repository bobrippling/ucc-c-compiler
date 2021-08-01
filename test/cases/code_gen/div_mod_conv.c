// RUN: %ocheck 0 %s -DDIV_SIGNED
//
// RUN: %ucc -target x86_64-linux -S -o %t %s -UDIV_SIGNED
//
// -- mul:
// RUN:   grep 'shll $1, %%eax' %t
// RUN: ! grep 'addl.*%%eax' %t
// RUN: ! grep 'mul' %t
// -- div:
// RUN:   grep 'shrl $1, %%eax' %t
// -- mod:
// RUN:   grep 'andl $3, %%eax' %t
// -- div + mod:
// RUN: ! grep 'div' %t

void abort(void) __attribute__((noreturn));

// uppercase function names to avoid grep conflict when looking for instructions

MUL(unsigned i)
{
	return 2 * i;
}

#ifdef DIV_SIGNED
DIV(int i)
#else
DIV(unsigned i)
#endif
{
	return i / 2;
}

MOD(unsigned i)
{
	return i % 4;
}

main()
{
#include "../ocheck-init.c"
	if(MUL(2) != 4)
		abort();

	if(DIV(-3) != -1)
		abort();

	if(MOD(3) != 3)
		abort();
	if(MOD(2) != 2)
		abort();
	if(MOD(8) != 0)
		abort();
	if(MOD(7) != 3)
		abort();
	if(MOD(9) != 1)
		abort();

	return 0;
}
