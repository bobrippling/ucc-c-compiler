// RUN: %ocheck 3 %s

__attribute((always_inline))
f()
{
	goto a;
a:
	return 3;
}

main()
{
	goto a;
b:
	printf("hi\n");
	return f();

a:
	goto b;
}
