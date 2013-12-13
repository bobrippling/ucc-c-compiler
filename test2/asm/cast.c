// RUN: %ucc -c %s
// RUN: %ucc -S -o- %s | grep 'movz'

unsigned long f()
{
	unsigned char c;
	return c;
}
