// RUN: %ocheck 0 %s
//
// RUN: %ucc -target x86_64-linux -S -o %t %s
// RUN: grep 'shll $1, %%eax' %t
// RUN: %stdoutcheck %s < %t
//
// STDOUT-NOT: /addl.*%%eax/

void abort(void) __attribute__((noreturn));

f(unsigned i)
{
	return 2 * i;
}

g(int i)
{
	return i / 2;
}

main()
{
	if(f(2) != 4)
		abort();

	if(g(-3) != -1)
		abort();

	return 0;
}
