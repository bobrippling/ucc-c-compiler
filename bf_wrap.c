#include "bf.h"

pbf(struct half_bytes *p)
{
	printf("%d %d %d %d\n",
		p->f_1, p->bf_1, p->bf_2, p->f_2);
}

main()
{
	struct half_bytes a;

	a.f_1 = 1;
	a.bf_1 = 2;
	a.bf_2 = 3;
	a.f_2 = 4;

	pbf(&a);

	write_bf(&a);

	pbf(&a);

	//a.b = 7;

	// *(char *)a
	// = 2 + 4 << 4
	// = 2 + (0b100 << 4)
	// = 2 + (0b1000000 << 4)
	// = 2 + 64
	// = 66 ???

	//printf("%d %d\n", sizeof a, *(unsigned char *)&a);
}
