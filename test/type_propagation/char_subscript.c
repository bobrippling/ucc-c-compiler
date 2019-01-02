// RUN: %check %s -Wchar-subscripts

f(char c, int x[])
{
	return x[c] + *(x + c); // CHECK: warning: array subscript is of type 'char'
}

g(unsigned char c, int x[])
{
	return x[c] + *(x + c); // CHECK: !/warning:.*char/
}

h(signed char c, int x[])
{
	return x[c] + *(x + c); // CHECK: !/warning:.*char/
}
