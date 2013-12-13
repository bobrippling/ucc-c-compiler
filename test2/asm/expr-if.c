// RUN: %ucc -c %s

fclose2(x)
{
	x ? 1 : 2;
	x ?:5;
	q();
}
