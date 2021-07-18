// RUN: %check -e %s

void init(int (*p)[])
{
	p[0][0] = 1; // CHECK: error: arithmetic on pointer to incomplete type int[]
}
