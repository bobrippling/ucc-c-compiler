// RUN: %check -e %s

int f(); // CHECK: /note: previous definition/

static int caller()
{
	return f();
}

static int f() // CHECK: /error: mismatching definitions of "f"/
{
	return 3;
}
