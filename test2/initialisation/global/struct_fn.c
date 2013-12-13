// RUN: %layout_check %s
struct __FILE
{
	int (*f_read)();
} stdin  = { (void *)0 };
