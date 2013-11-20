// RUN: %layout_check %s
struct __FILE
{
	int (*f_read)();
} _stdin  = { (void *)0 };
