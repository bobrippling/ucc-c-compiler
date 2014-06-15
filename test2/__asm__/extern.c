// RUN: %ucc -S -o- %s | grep renamed
f()
{
	extern int i __asm("renamed");

	i = 2;
}
