// RUN: %check --only %s

int printf(const char *, ...) __attribute__((format(printf, 1, 2)));

not_variadic(char *) __attribute__((format(printf, 1, 2))); // CHECK: warning: variadic function required for format attribute

just_str_check(char *, ...) __attribute((format(printf, 1, 0)));

bad_v_idx(int, int, char *, ...) __attribute__((format(printf, 3, 3))); // CHECK: warning: variadic argument out of bounds (should be 4)

bad_v_idx2(int, int, int, int, char *, ...)
	__attribute__((format(printf, 5, 4))); // CHECK: warning: variadic argument out of bounds (should be 6)

bad_fmt_idx(char *, int, ...)
	__attribute__((format(printf, 2, 3))); // CHECK: warning: format argument not a string type

main()
{
	printf("%d %s\n", 5, "hello"); 
	printf("%d %s\n", 5, L"hello"); // CHECK: warning: %s expects a 'char *' argument, not 'int *'
	printf("%d %s\n", "hello", 5);  // CHECK: warning: %d expects a 'int' argument, not 'char *'
	// CHECK: ^warning: %s expects a 'char *' argument, not 'int'

	not_variadic("hi");

	just_str_check("hi %"); // CHECK: warning: incomplete format specifier
	just_str_check("hi %d");

	bad_v_idx(1, 2, "hi %s", 3);
	bad_v_idx2(5, 5, 5, 5, "yo %d", "hi");

	// shouldn't get warnings on functions with bad format(printf,...) specs:
	bad_fmt_idx("hi %s", 3, 5);
}
