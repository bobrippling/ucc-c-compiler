extern int printf(char *, ...);

void (^pb)() = ^{ return; };

int i, j;

int *(^funcs[])() = {
	[0 ... 1] = ^{ printf("1!\n"); return &i; },
	^{ printf("2!\n"); return &j; },
};

void *f(int x)
{
	return funcs[x]();
}

main()
{
	printf("&i = %p\n", &i);
	printf("&j = %p\n", &j);
	for(int i = 0; i < 3; i++)
		printf("f(%d) = %p\n", i, f(i));
}
