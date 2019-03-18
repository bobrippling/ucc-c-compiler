// RUN: %check %s

#define CAST(x) (x)

int a;

int *f(int x)
{
	x = CAST(int)&a; // CHECK: warning: cast from 'int *' to smaller integer type 'int'
	return CAST(int *)x; // CHECK: warning: cast to 'int *' from smaller integer type 'int'
}
