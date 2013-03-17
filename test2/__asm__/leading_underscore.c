// RUN: %ucc -fleading-underscore %s
// RUN: ! %ucc -S -o- -fleading-underscore %s | grep '_no_leading'
// RUN: %ucc -S -o- -fleading-underscore %s | grep '_main'

int i asm("no_leading"), j;

extern g();

main()
{
	extern h();

	f(), g(), h();

	i = j = 2;
	return i;
}
