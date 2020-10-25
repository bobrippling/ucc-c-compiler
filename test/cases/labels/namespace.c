// RUN: %ocheck 0 %s
int printf(const char *, ...) __attribute__((format(printf, 1, 2)));

typedef int i;

void yo()
{
	struct i {i i;} i;
i:	i.i = 3;
	 printf("%d\n", i.i);
}

f()
{
	goto a;
a:;
}

main()
{
	goto a;
a: f();
	 return 0;
}
