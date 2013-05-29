#include "bf.h"

main()
{
	struct half_bytes a;

	a.f_1 = -1UL;
	a.bf_1 = -1UL;
	a.bf_2 = -1UL;
	a.f_2 = -1UL;

	//a.f_1 = 1; // BUG
	//a.bf_1 = 2;
	//a.bf_2 = 3;
	//a.f_2 = 4;

	pbf(&a);
}
