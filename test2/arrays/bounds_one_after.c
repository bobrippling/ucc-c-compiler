// RUN: %ucc -o /dev/null -S %s 2>&1 | %check %s

f()
{
	static int a[10];
	return a + 10; // CHECK: !/index.*bound/
}
