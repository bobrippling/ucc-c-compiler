typedef _Bool bool;
typedef int int32_t;

RET add_and_check_overflow(int32_t *a, int32_t b)
{
	bool result;

	// + - need to lval2rval the operand before inline asm
	__asm__(
			"addl %2, %1\n\t"
			"seto %0 // TODO: %%b0"
			: "=q" (result), "+g" (*a)
			: "r" (b));

	return result;
}
