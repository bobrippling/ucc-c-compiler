// TEST: target !darwin
// RUN: %ocheck 0 %s
// RUN: %check --only %s

void abort();

__attribute__((weak))
void f();

void g();

extern int w __attribute__((weak));
int z = 1;

// test constant-expr ness
void (*p[])() = {
	f,
	g,
	&f,
	&g,
};

void g()
{
	z = 0;
}

int main()
{
	if(f)
		abort();

	if(&w)
		abort();

	if(f &&& w)
		f(w);

	if(&z) // CHECK: warning: address of lvalue (int) is always true
		g();

	return z;
}
