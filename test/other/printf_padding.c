#include <stdio.h>
int main()
{
	int i;
	for(i = 0; i < 10; i++)
		printf("%02d\n", i);
	for(i = 0; i < 10; i++)
		printf("%04d\n", i);
}
