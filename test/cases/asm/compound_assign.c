// RUN: %ucc -c %s

int f(int *ap)
{
	int n;

	n = (ap += 1);
}
