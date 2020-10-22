// RUN: %ocheck 0 %s

_Noreturn void abort(void);

static int checks;

struct A
{
	int x;
};

static int comma()
{
	struct A a = { 3 };
	checks++;
	return (0, a).x;
}

static void check_comma()
{
	if(comma() != 3)
		abort();
	checks++;
}

struct B
{
	int ar[2];
	int other;
};

static struct B a(void)
{
	return (struct B){ { 1 }, 3 };
}

static void check_dot()
{
	int o = a().other;

	if(o != 3)
		abort();
	checks++;
}

struct C
{
	int i, j, k;
	char c;
};

static int pred(){ return 1; }

static void check_C(
		struct C const *p,
		int i, int j, int k, char c)
{
	if(p->i != i
	|| p->j != j
	|| p->k != k
	|| p->c != c)
	{
		abort();
	}
}

static void check_C_variants()
{
	struct C y = { .c = 'y' }, z = { .c = 'z' };
	struct C x = pred() ? y : z;

	/*struct C cst = (struct C)x;*/

	struct C comma = (y, z);

	struct C exp_stmt = ({ struct C sub = { .k = 1, .c = 's' }; sub; });

	struct C init = (struct C){ .j = 1, .c = 'i' };

	(void)x;

	check_C(&y,        0, 0, 0, 'y');
	check_C(&z,        0, 0, 0, 'z');
	check_C(&x,        0, 0, 0, 'y');
	check_C(&comma,    0, 0, 0, 'z');
	check_C(&exp_stmt, 0, 0, 1, 's');
	check_C(&init,     0, 1, 0, 'i');
	checks++;
}

typedef struct
{
	int x[1];
} D;

static D *f()
{
	static D a = { 1 };
	return &a;
}

static void check_D()
{
	f()->x[0] = 3;

	if(f()->x[0] != 3)
		abort();
	checks++;
}

struct E
{
	int x[5];
};

static void check_E()
{
	(struct E){
		{ 1, 2, 3, 4, 5 },
	};

	struct E x = { 0 };

	(void)x;
	checks++;
}

int main()
{
	check_comma();
	check_dot();
	check_C_variants();
	check_D();
	check_E();

	return checks == 6 ? 0 : 1;
}
