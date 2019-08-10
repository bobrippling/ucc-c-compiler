// RUN: %check --only -e --prefix=error %s -fno-builtin
// RUN: %check --only %s

unsigned long strlen(const char *);

main()
{
	_Static_assert(
			__builtin_constant_p(strlen("hi")), // CHECK-error: error: static assert: not an integer constant expression
			// CHECK-error: ^error: implicit declaration of function "__builtin_constant_p"
			"no builtin");
}
