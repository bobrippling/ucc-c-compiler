// RUN: %ocheck 0 %s
// RUN: %ocheck 0 %s -fstack-protector-all

typedef __builtin_va_list va_list;

void abort(void);
int strcmp(char *, char *);

void print_str(
		// these extra arguments test the reg_save_area and gp_offset logic
		int one, int two, int three,
		char *first, ...)
{
	va_list l;

	(void)one;
	(void)two;
	(void)three;

	__builtin_va_start(l, first);
	char *p = __builtin_va_arg(l, char *);
	__builtin_va_end(l);

	if(strcmp(first, "hi\n"))
		abort();

	if(strcmp(p, "yo\n"))
		abort();
}

main()
{
	print_str(1, 2, 3, "hi\n", "yo\n");
	return 0;
}
