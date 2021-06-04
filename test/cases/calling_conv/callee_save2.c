// RUN: %ocheck 11 %s
// RUN: %ucc -S -o- -target x86_64-linux %s -fno-inline-functions | grep -F 'movl %%eax, %%ebx'

g()
{
	return 5;
}

h()
{
	return 6;
}

f(a)
{
	return a;
}

main()
{
#include "../ocheck-init.c"
	return f(g() + h());
}
