// RUN: %ucc -c %s

f()
{
	int (*p)();
	p();
}
