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

static const A s1 = { 1, 2, 3 };
static const A s2 = { 5, 6, 7 };
static A s3;

void g()
{
	A *p = &s3;
	int c = 3;

	*p = *(c ? &s1 : &s2); // stage1 compiler balks on this
}

static const int i1, i2;
static int i3;

void g_does_the_bug_happen_without_lvaluestruct_logic()
{
	typedef int A;
	A *p = &i3;
	int c = 3;

	__builtin_memcpy(p, (c ? &i1 : &i2), sizeof(*p));
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
