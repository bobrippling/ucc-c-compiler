// RUN: %check --only %s -Wno-cast-qual

int const_only(char *a, const char *b)
{
	// ensure we detect this either way around
	return a == b // CHECK: warning: mismatching types, comparison lacks a cast
		|| b == a; // CHECK: warning: mismatching types, comparison lacks a cast
}

int main()
{
	return (char *)0 == (int *)5; // CHECK: warning: comparison of distinct pointer types, comparison lacks a cast
}
