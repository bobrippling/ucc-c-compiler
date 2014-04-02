struct A
{
	double x, y;
} a;

struct A f(int cond)
{
	cond ? a : 0;
}

