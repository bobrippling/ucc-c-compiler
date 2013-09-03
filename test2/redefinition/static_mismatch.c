// RUN: %check -e %s

int f(); // CHECK: /error: mismatching definitions of "f"/

static int caller()
{
	return f();
}

static int f() // CHECK: /note: other definition/
{
	return 3;
}
