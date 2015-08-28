enum A
{
	X,
	Y
}; // ARF????? warning: unused variable '(null)'

void f(enum A a)
{
	a = 0;
	a = 2;
}
