// RUN: %ocheck 0 %s

extern _Noreturn void abort(void);

int g(int a, int b)
{
	if(b)
		abort();
	if(a != 5)
		abort();

	return 72;
}

dup(int x)
{
	return x + 1;
}

f(int a, int b)
{
	// bug here - register saving between jumps (?:)
	return g(a, b ? dup(b) : 0);
}

main()
{
	f(5, 0);
}

/*
typedef struct opaque *id;

id (*print_and_ret(id x, ...))(id, ...)
{
	printf("%p\n", x);
	return print_and_ret;
}

extern id (*_imp(id x))()
{
	return print_and_ret;
}

typedef char *SEL;

id f(id self, SEL _cmd, id arg)
{
	id local1, local2;

	return (
			local1 = (id)self,
			_imp(local1)(
				local1,
				arg
				?
					local2 = arg,
					_imp(local2)(local2)
				: (id)0));
}

main()
{
	f(3, "hi", 42);
}
*/
