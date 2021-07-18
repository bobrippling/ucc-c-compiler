// RUN: %check -e %s

typedef enum E {} E; // CHECK: error: expecting token identifier, got '}'

E f(E e)
{
	return e;
}

int main()
{
	return f(3);
}
