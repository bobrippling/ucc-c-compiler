// RUN: %asmcheck %s

tim() asm("ma"  "in");

int y asm("hi") __attribute(()) = 3;

tim()
{
	y += 2;
	return y;
}
