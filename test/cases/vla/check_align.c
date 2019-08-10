// RUN-disabled: %ucc -o %t %s "$(dirname %s)"/check_align_asm.s
// RUN-disabled: %t
//
// test disabled, passes on Darwin, fails on Linux - timeout was hiding the crash

extern void check_align() __asm("check_align");

a()
{
	int i = 1;
	short c[i];

	c[0] = 0;

	check_align(c, i);
}

b()
{
	int i = 3;
	char c[i];

	c[0] = 0;
	c[1] = 0;
	c[2] = 0;

	check_align(c, i);
}

main()
{
	a();
	b();

	return 0;
}
