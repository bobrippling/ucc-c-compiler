// RUN: %ucc -c -o %t.o %s -fpic
// below ensures we link with the system libs
// RUN: cc -o %t %t.o -fpic
// RUN: %t | %output_check before 'hi 5 hello' after

#define va_list __builtin_va_list
#define va_start __builtin_va_start
#define va_arg __builtin_va_arg
#define va_end __builtin_va_end

final_greeting(char *fmt, ...)
{
	va_list l;
	printf("before\n");
	va_start(l, fmt);
	vprintf(fmt, l); // tests ABI compatability of __builtin_va_list
	va_end(l);
	printf("after\n"); // use printf(), not write() - vprintf() might cache
}

main()
{
	final_greeting("hi %d %s\n", 5, "hello");
}
