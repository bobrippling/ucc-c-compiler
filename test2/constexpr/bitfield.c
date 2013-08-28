// RUN: %check -e %s

struct R
{
	int i : g(); // CHECK: /error: constant expression required for field width/
};
