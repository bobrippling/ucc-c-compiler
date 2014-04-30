// RUN: %ucc -o %t %s
// RUN: %t | %output_check "hello" "hi" "greetings" "hola"

printv(const char *a, __builtin_va_list l)
{
	extern write(), strlen();
	while(a){
		write(1, a, strlen(a));
		write(1, "\n", 1);
		a = __builtin_va_arg(l, __typeof(a));
	}
	return 0;
}

print(const char *a, ...)
{
	__builtin_va_list l;
	__builtin_va_start(l, a);
	int const t = printv(a, l);
	__builtin_va_end(l);
	return t;
}

main()
{
	print("hello", "hi", "greetings", "hola", 0L);
	return 0;
}
