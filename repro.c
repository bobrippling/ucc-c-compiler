char *strcpy();

char *f(void)
{
	static char buf[16];

	if(!buf[0]){ // stage2 compiler balks on this, bug is the memcpy @ expr_op.c:197
		strcpy(buf, "hi");
	}

	return buf;
}

typedef struct A
{
	long a, b, c;
} A;

void g()
{
	A d;
	A s1 = { 1, 2, 3 };
	A s2 = { 5, 6, 7 };
	A *p;
	int c = 3;
	p = &d;

	*p = *(c ? &s1 : &s2); // stage1 compiler balks on this
}

/*
// stage2 compiler balks on:
void f(char *p)
{
	p++;
}

// stage1 compiler balks on:
int something_causing_spill(void);
char *get(void);

void g()
{
	char *p = get();
	int x = 3;
	*p += (x ? 1 : -1); // something causing spill, but not a call (callee save regs are fine)
}
*/
