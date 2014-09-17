// RUN: %check %s

const void f() // CHECK: warning: function has qualified void return type (const)
{
}

const int g() // CHECK: warning: const qualification on return type has no effect
{
	return 3;
}
