int var14;
int func14(int p1)
{
	asm ("add %1, %0"
		:"+r" (p1) : "rm" (var14) : "cc");
	/* should use var14 directly here (with -fno-pic) */
	return foo(p1);
}
