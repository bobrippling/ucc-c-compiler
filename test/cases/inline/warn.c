// RUN: %check %s -Winline -fno-semantic-interposition

inline int f(int a, ...)
{
	return 3;
}

int main()
{
	return f(1); // CHECK: warning: can't inline call: call to variadic function
}
