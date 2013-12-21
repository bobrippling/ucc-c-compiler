// RUN: %ucc -S -o/dev/null %s

f(double d)
{
	f((double)d);
}
