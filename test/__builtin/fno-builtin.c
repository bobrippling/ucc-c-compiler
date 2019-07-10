// RUN: %check -e %s -fno-builtin
// RUN: %check %s

unsigned long strlen(const char *);

main()
{
	_Static_assert(
			__builtin_constant_p(strlen("hi")), // CHECK: error: static assert: not an integer constant expression
			"no builtin");
}
