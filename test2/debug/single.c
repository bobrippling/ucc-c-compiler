// RUN: %ucc -g %s -o %t

struct A
{
	int i, j;
	struct A *next;
} glob_a = { 1, 2 };

struct A *a(void)
{
	static struct A yo = { 5, 6 };
	return &yo;
}

pa(struct A *p);

int main(int argc, char **argv)
{
	int hi = 1;
	{
		int hi = 5;

		struct A *f = a();
		glob_a = *f;
		f->j = 2;
		pa(f);
	}
	{
		int hi = 2;
	}
	return hi;
}
