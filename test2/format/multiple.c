// RUN: %check %s

int printf(const char *, ...) __attribute__((format(printf, 1, 2))); // CHECK: !/warn|err/

not_variadic(char *) __attribute__((format(printf, 1, 2))); // CHECK: /error: "f2" is not a variadic function/

just_str_check(char *, ...) __attribute((format(printf, 1, 0))); // CHECK: !/warn|err/

bad_v_idx(int, int, char *, ...) __attribute__((format(printf, 3, 3))); // CHECK: /error: variadic argument 3 not '...'/

bad_v_idx2(int, int, int, int, char *, ...)
	__attribute__((format(printf, 5, 4))); // CHECK: /error: variadic argument (4) before format argument (5)/

bad_fmt_idx(char *, int, ...)
	__attribute__((format(printf, 2, 3))); // CHECK: /error: format index not 'char *'/

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
	bad_fmt_idx("hi %s", 3, 5); // CHECK: !/warn/
}
