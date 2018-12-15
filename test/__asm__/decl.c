// RUN: %asmcheck %s

tim() __asm("ma"  "in");

int y __asm("hi") __attribute(()) = 3;

tim()
{
	y += 2;
	return y;
}
