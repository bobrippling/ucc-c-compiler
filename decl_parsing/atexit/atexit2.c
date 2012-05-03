static int (*fs[3])();
static int f_i;

add(int (*f)())
{
	fs[f_i++] = f;
}

run()
{
	for(int i = 0; fs[i]; i++)
		fs[i]();
}

a(){}
b(){}

main()
{
	printf("%p\n", fs);
	//add(a);
	//add(&b);
	//run();
}
