// RUN: %check --only %s -Wno-cast-qual

int const_only(char *a, const char *b)
{
	return a == b // no warning
		|| b == a; // no warning
}

void assign(char *a, const char *b)
{
	a = b; // CHECK: warning: mismatching types, assignment
	b = a;
}

int main()
{
	return (char *)0 == (int *)5; // CHECK: warning: distinct pointer types in comparison lacks a cast
}
