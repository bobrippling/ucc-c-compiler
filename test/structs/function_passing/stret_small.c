// RUN: %ocheck 0 %s
void abort(void) __attribute__((noreturn));

struct A
{
	int x;
};

void clobber_args()
{
}

struct A f(void)
{
	clobber_args(0, 1, 2);
	return (struct A){ 99 };
}

int main()
{
	struct A a;

	a = f();

	if(a.x != 99)
		abort();

	return 0;
}
