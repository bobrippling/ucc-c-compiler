// RUN: %ucc -c %s

f(int (*pa)[])
{
	return (*pa)[1];
}
