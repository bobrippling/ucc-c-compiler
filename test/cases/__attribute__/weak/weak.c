// don't run this test - only certain targets support weak variables

// RUN: %ucc -S -o- -target x86_64-darwin %s | grep weak | %stdoutcheck --prefix=darwin %s
// STDOUT-darwin: .weak_reference _w
// STDOUT-darwin: .weak_reference _f

// RUN: %ucc -S -o- -target x86_64-linux %s | grep weak | %stdoutcheck --prefix=linux %s
// STDOUT-linux: .weak f
// STDOUT-linux: .weak w

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
	/*
	if(f)
		abort();

	if(&w)
		abort();

	if(f &&& w)
		f(w);
	*/

	if(&z) // CHECK: warning: address of lvalue (int) is always true
		g();

	//return z;
}
