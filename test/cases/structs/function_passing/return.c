// RUN: %check --only %s -Wall -Wextra -Waggregate-return

int f()
{
	return 3;
}

struct A
{
	int i;
} g();

int main()
{
	g(); // CHECK: warning: called function returns aggregate (struct A)
}

struct A h() // CHECK: warning: function returns aggregate (struct A)
{
	return (struct A){ 1 };
}
