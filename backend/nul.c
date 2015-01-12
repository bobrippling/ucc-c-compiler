#define ADDR *(volatile int *)0

f()
{
	ADDR = 3;
	return 2 + ADDR;
}
