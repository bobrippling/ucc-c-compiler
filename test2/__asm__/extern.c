// RUN: %ucc -S -o- %s | grep renamed
f()
{
	extern int i asm("renamed");

	i = 2;
}
