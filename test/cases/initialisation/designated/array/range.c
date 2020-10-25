// RUN: %ucc -c %s
f()
{
	int x[] = { [0 ... 9] = 3 };
}
