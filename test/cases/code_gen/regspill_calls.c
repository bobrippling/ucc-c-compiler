// RUN: %ocheck 5 %s
void abort(void) __attribute__((noreturn));

u(){ return 1; }
v(){ return 2; }
w(){ return 3; }
x(){ return 4; }
y(){ return 5; }
z(){ return 6; }

g(u, v, w, x, y, z)
{
	if(u != 1) abort();
	if(v != 2) abort();
	if(w != 3) abort();
	if(x != 4) abort();
	if(y != 5) abort();
	if(z != 6) abort();
}

main()
{
	long local = 5;
	g(u(), v(), w(), x(), y(), z());
	g(u(), v(), w(), x(), y(), z());
	return local;
}
