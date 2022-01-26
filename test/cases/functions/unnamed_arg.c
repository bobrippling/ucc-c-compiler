// RUN: %check --prefix=c11 --only -e %s -std=c11
// RUN: %check --prefix=c2x --only    %s -std=c2x

int f(int, int j) // CHECK-c11: error: argument 1 in "f" is unnamed
{
	return j;
}

int main()
{
	return f(3, 2);
}
