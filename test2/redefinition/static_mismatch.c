// RUN: %check -e %s

int f(); // CHECK: note: previous definition

static int caller()
{
	return f();
}

static int f() // CHECK: error: static redefinition of non-static "f"
{
	return 3;
}
