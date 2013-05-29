#include "bf.h"

pbf(struct half_bytes *p)
{
	printf("%d %d %d %d\n",
		p->f_1, p->bf_1, p->bf_2, p->f_2);
}
