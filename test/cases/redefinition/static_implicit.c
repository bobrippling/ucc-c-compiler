// RUN: %check -e %s

static int caller()
{
	/* this assumes a non-static f(),
	 * which then mismatches with the actual definition */
	return f(); // CHECK: note: previous definition here
}

static int f() // CHECK: error: static redefinition of non-static "f"
{
	return 3;
}
