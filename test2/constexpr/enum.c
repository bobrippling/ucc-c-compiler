// RUN: %check -e %s

enum X
{
	A = f() // CHECK: /error: integral constant expected/
};
