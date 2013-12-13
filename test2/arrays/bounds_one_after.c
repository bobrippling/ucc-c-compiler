// RUN: %check %s

f()
{
	static int a[10];
	return a + 10; // CHECK: !/index.*bound/
}
