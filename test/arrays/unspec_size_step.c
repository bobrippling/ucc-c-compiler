// RUN: %ucc -c -o %t %s

struct A
{
	int y, x;
};

extern struct A a[];

f()
{
	a[1].x = 2;
}
