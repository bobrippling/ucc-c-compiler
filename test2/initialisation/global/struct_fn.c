// RUN: %ucc -c %s
// RUN: %asmcheck %s
struct __FILE
{
	int (*f_read)();
} _stdin  = { (void *)0 };
