// RUN: %ucc -o %t %s
// RUN: %t
// RUN: %t | %output_check a b c
int printf(const char *, ...) __attribute__((format(printf, 1, 2)));
void abort(void) __attribute__((noreturn));

void (*fs[3])(void);
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

	if(sizeof fs != 3 * 8)
		abort();

	return 0;
}
