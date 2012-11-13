// RUN: %ucc -c %c -o /dev/null
// RUN: %ucc -c %c -o /dev/null 2>&1 | %check %s

f(int *restrict a, int *restrict b)
{
	a = b + 1; // CHECK: /warning: restrict pointers in assignment/

	a = (void *)(b[0] + 1); // CHECK: !/warn/

	*a = *b; // CHECK: !/warn/

	__builtin_trap();
}
