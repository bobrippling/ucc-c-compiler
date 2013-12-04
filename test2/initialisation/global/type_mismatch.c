// RUN: %layout_check %s

int *p = 5;

f()
{
	static int *p = 5;
}
