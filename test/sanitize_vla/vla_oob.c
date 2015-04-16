#include "vla_sup.h"

f(int j)
{
	short ar[g() + j];

	fill(ar, sizeof ar, 3);

	return ar[j * 2];
}
