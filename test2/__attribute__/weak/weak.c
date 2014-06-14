// TEST: target !darwin
// RUN: %ocheck 0 %s

__attribute__((weak))
void f();

extern int w __attribute__((weak));
int z = 1;

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

	if(&z)
		g();

	return z;
}
