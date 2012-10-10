typedef int size_t;

old(argc, argv)
    int argc;
    char **argv;
{
}

old_int(a, b)
{
	return a + b;
}

old_with_tdef(x)
	size_t x;
{
}

new_with_typedef(size_t);
new_with_typedef(size_t a);
new_with_typedef(size_t a)
{
}

normal(int a)
{
}

main(){}
