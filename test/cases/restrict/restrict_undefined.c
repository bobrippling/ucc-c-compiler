// RUN: %ucc -c %s -o /dev/null
// RUN: %check %s -Wno-int-ptr-conversion -Wno-ptr-int-conversion

f(int *restrict a, int *restrict b)
{
	a = b + 1; // CHECK: /warning: restrict pointers in assignment/

	a = (void *)(b[0] + 1); // CHECK: !/warn/

	*a = *b; // CHECK: !/warn/

	__builtin_trap();
}
