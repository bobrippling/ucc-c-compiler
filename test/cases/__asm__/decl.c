// RUN: %ucc -target x86_64-linux -S -o %t %s

// RUN:   grep main %t
// RUN:   grep hi %t
// RUN: ! grep tim %t
// RUN: ! grep '\by\b' %t

tim() __asm("ma"  "in");

int y __asm("hi") __attribute(()) = 3;

tim()
{
	y += 2;
	return y;
}
