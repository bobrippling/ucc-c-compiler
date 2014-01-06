//#include <stdbool.h>
//#include <stdarg.h>
//#include <stdnoreturn.h>

struct A
{
	int i, j;
	//struct A *next;
} glob_a = { 1, 2 };

struct A *a(void)
{
	static struct A yo = { 5, 6 };
	return &yo;
}

pa(struct A *p)
{
	printf("{%d, %d}\n", p->i, p->j);
}

int main(int argc, char **argv)
{
	int hi = 1;
	{
		int hi = 5;
		printf("%d\n", hi);

		struct A *f = a();
		glob_a = *f;
		f->j = 2;
		pa(f);
	}
	{
		int hi = 2;
		printf("%d\n", hi);
	}
	return hi;
}
