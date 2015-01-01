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
