// RUN: %ucc -fsyntax-only %s
// RUN: %check %s

void  __attribute__((__noreturn__)) _exit(int x);

void _exit(int x) __attribute__((__noreturn__));

void _exit(int x) __attribute__((__noreturn__)) // CHECK: !/warning:.*returns/
{
	__builtin_trap();
}

int atoi(const char *);
