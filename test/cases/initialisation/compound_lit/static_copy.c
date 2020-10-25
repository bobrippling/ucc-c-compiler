// RUN: %check %s -Wnonstandard-init
// RUN: %ocheck 0 %s

struct A
{
	int i, j;
};

struct A a = (struct A){ 1, 2 }; // CHECK: warning: global brace initialiser contains non-standard constant expression

_Noreturn void abort(void);


struct Nested
{
	struct A a;
	struct A as[2];
};

struct Nested n = (struct Nested){ // CHECK: warning: global brace initialiser contains non-standard constant expression
	(struct A){ 1, 2 }, // CHECK: warning: global brace initialiser contains non-standard constant expression
	{
		(struct A){ 3, 4 },
		(struct A){ 5, 6 },
	}
};

struct Nested ns[] = { // CHECK: warning: global brace initialiser contains non-standard constant expression
	{
		(struct A){ 1, 2 },
		(struct A){ 3, 9 },
		(struct A){ 5, 6 },
	},
	{
		(struct A){ 1, 2 },
		(struct A){ 9, 4 },
		(struct A){ 5, 6 },
	}
};

void check_a(struct A *p, int i, int j)
{
	if(p->i != i)
		abort();
	if(p->j != j)
		abort();
}

void check_n(struct Nested *p, int e[static 6])
{
	check_a(&p->a, e[0], e[1]);
	check_a(&p->as[0], e[2], e[3]);
	check_a(&p->as[1], e[4], e[5]);
}

int main()
{
	check_a(&a, 1, 2);

	check_n(&n, (int[]){ 1, 2, 3, 4, 5, 6 });

	check_n(&ns[0], (int[]){ 1, 2, 3, 9, 5, 6 });
	check_n(&ns[1], (int[]){ 1, 2, 9, 4, 5, 6 });

	return 0;
}
