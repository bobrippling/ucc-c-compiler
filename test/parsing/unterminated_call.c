// RUN: %check -e %s

f()
{
	g(1,); // CHECK: error: expression expected, got ')'
}
