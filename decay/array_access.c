#include <stdio.h>
#define P(x) printf("%d\n", x)

main()
{
	char a[80];
	char *p_first = a;
	char (*p_a)[80] = &a;
	int i;

	for(i = 0; i < 80; i++)
		a[i] = i;

	i = 5;

	P(a[i]);       // i'th element of a
	P(p_first[i]); // ditto
	P((*p_a)[i]);  // ditto... wtf


	P(*p_a[i]);    // wat
	P(p_a[i]);     // some random stack item
}
