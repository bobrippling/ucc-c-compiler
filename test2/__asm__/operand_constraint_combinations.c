// RUN: %ocheck 0 %s

#define CHECK_ASM(cstraint_out, cstraint_in) \
	__asm("movl %1, %0" : cstraint_out(i) : cstraint_in(5)); \
	if(i != 5) \
		printf("failed on %s:%d\n", __FILE__, __LINE__)

int printf(const char *, ...);

main()
{
	int i;

	CHECK_ASM("=r", "r");
	CHECK_ASM("=g", "r");
	CHECK_ASM("=m", "r");

	CHECK_ASM("=r", "g");
	CHECK_ASM("=g", "g");
	CHECK_ASM("=m", "g");

	CHECK_ASM("=r", "m");
//CHECK_ASM("=g", "m");
//CHECK_ASM("=m", "m");

	CHECK_ASM("=r", "i");
	CHECK_ASM("=g", "i");
	CHECK_ASM("=m", "i");

	return 0;
}
