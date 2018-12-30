// RUN: %check -e %s

f()
{
	__label__ l;
	__label__ h; // CHECK: error: label 'h' undefined
	// ^ multiple __label__ entries
l:;
}
