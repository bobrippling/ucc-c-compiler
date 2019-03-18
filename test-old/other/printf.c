#include <stdio.h>

main()
{
	int i;
	for(i = 0; i < 10; i++)
		printf("%d %s %c %x\n", i, "hi", 'a', i + 1);
}
