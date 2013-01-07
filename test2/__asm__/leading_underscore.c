// RUN: %ucc -fleading-underscore %s
// RUN: %ucc -S -o- -fleading-underscore %s | grep '[^_]no_leading'
// RUN: %ucc -S -o- -fleading-underscore %s | grep '_main'

int i asm("no_leading");

main()
{
	i = 2;
	return i;
}
