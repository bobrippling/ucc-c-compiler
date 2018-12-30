// RUN: %layout_check %s

int *p = 5;

f()
{
	static __attribute((used)) int *p = 5;
}
