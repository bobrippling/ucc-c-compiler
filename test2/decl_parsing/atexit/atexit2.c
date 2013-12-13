// RUN: %ucc -o %t %s
// RUN: %t | %output_check a b
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

a(){printf("a\n");}
b(){printf("b\n");}

main()
{
	//printf("%p\n", fs);
	add(a);
	add(&b);
	run();
}
