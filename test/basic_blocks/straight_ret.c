// RUN: %jmpcheck %s

main()
{
	goto a;

b:
	goto c;

a:
	goto b;

c:
	return 3;
}
