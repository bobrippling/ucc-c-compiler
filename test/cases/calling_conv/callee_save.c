// RUN: %ucc -S -o %t %s
// RUN: grep -F 'not saving reg 1 - callee' %t

typedef int f(void);

f a, b, c;

main()
{
	// a() is cached on-stack,
	// b() is cached in ebx
	return a() + b() + c();
}
