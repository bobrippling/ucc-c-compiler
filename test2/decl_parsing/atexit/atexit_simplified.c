// RUN: %ucc -o %t %s
// RUN: %t | %output_check a b c
// RUN: %ucc -Xprint %s | grep -F 'fs.* size 24 bytes'
void (*fs[3])(void);
int f_i;

add(void (*f)())
{
	fs[f_i++] = f;
}

run()
{
	int i;
	printf("fs = %p\n", fs);
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
