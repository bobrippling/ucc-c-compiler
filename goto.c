f(void *arg)
{
	static const void *const p[] = {
		&&a, &&b, &&c
	};

	goto **p;

a:
	return 2;
b:
	return 9;
c:
	return 5;
}

g(void *arg)
{
	&&a, &&b;

	goto *arg;

a:
	return 2;
b:
	return 9;
}
