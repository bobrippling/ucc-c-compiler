// RUN: %check %s

f(char *a, const char *b)
{
	return a - b; // CHECK: !/warn/
}
