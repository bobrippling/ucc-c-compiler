// RUN: %check -e %s

typedef int f;

int f() // CHECK: /error: incompatible redefiniton of 'f'/
{
	return 3;
}

int main()
{
	f i;
	return f();
}
