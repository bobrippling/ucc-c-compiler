// RUN: %ucc -c %s
// RUN: %ucc -S -o- %s | grep 'movzx'

unsigned long f()
{
	unsigned char c;
	return c;
}
