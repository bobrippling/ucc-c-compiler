void (*fs[3])(void); // only has 3 bytes resb'd
int f_i;

add(void (*f)())
{
	fs[f_i++] = f;
}

run()
{
	int i;
	for(i = 0; i < f_i; i++)
		fs[i]();
}

#define F(x) void x(){printf(#x "\n");}

F(a)
F(b)
F(c)

main()
{
	add(a);
	add(b);
	add(&c);

	run();
}
