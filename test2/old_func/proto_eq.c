// RUN: %check -e %s

int f(int a, char *b); // CHECK: note: previous definition

int f(a, b) // CHECK: error: mismatching definitions of "f"
	char *a;
	char *b;
{
	(void)a;
	(void)b;
}
