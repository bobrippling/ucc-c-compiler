typedef struct A {
	long a, b, c;
} A;

A f(A a)
{
	a.b += 2;
	return a;
}

int main()
{
	A a = f((A){ 1, 2, 3 });

	return a.b;
}
