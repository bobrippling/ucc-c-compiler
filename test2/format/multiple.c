// RUN: %check %s

int printf(const char *, ...) __attribute__((format(printf, 1, 2))); // CHECK: !/warn|err/

not_variadic(char *) __attribute__((format(printf, 1, 2))); // CHECK: /warning: variadic function required for format attribute/

just_str_check(char *, ...) __attribute((format(printf, 1, 0))); // CHECK: !/warn|err/

bad_v_idx(int, int, char *, ...) __attribute__((format(printf, 3, 3))); // CHECK: warning: variadic argument out of bounds (should be 4)

bad_v_idx2(int, int, int, int, char *, ...)
	__attribute__((format(printf, 5, 4))); // CHECK: warning: variadic argument out of bounds (should be 6)

bad_fmt_idx(char *, int, ...)
	__attribute__((format(printf, 2, 3))); // CHECK: warning: format argument not a string type

main()
{
	printf("%d %s\n", 5, "hello");  // CHECK: !/warn/
	printf("%d %s\n", 5, L"hello"); // CHECK: /warn/
	printf("%d %s\n", "hello", 5);  // CHECK: /warn/

	not_variadic("hi"); // CHECK: !/warn/

	just_str_check("hi %"); // CHECK: /warn/
	just_str_check("hi %d"); // CHECK: !/warn/

	bad_v_idx(1, 2, "hi %s", 3); // CHECK: !/warn/
	bad_v_idx2(5, 5, 5, 5, "yo %d", "hi"); // CHECK: !/warn/

	// shouldn't get warnings on functions with bad format(printf,...) specs:
	bad_fmt_idx("hi %s", 3, 5); // CHECK: !/warn/
}
