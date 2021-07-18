// RUN: %ucc -o %t %s -fsanitize=vla-bound -fsanitize-error=call=san_fail
// RUN:   %t x x x x
// RUN:   %t x x x
// RUN: ! %t x x
// RUN: ! %t x
// RUN: ! %t
//
// RUN: %ucc -S -o %t %s -fsanitize=vla-bound -DT=unsigned
// RUN:   grep '	ja .*Lblk' %t
// RUN: ! grep '	jg .*Lblk' %t
//
// RUN: %ucc -S -o %t %s -fsanitize=vla-bound -DT=int
// RUN:   grep '	jg .*Lblk' %t
// RUN: ! grep '	ja .*Lblk' %t

void san_fail(void)
{
	extern _Noreturn void exit(int);
	exit(5);
}

#ifndef T
#define T int
#endif

f(T i)
{
	char buf[i];
	buf[0] = 5;
	return buf[0];
}

main(int argc, char **argv)
{
	f(argc - 3);
}
