// RUN: %check -e %s

typedef struct A {
	long a,b,c,d;
} A;

int main()
{
	A a, b = { 5, 6, 7, 8 }, c = { 1, 2, 3, 4 };

	(a = b) = c; // CHECK: error: assignment to struct A - not an lvalue
}
