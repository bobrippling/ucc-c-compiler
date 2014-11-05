// RUN: %ocheck 3 %s

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
