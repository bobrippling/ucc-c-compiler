// RUN: %ucc -fsyntax-only %s

static int caller()
{
	/* should detect that this assumes a non-static f()...
	 * but we don't at the moment
	 */
	return f();
}

static int f()
{
	return 3;
}
