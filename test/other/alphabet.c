#include <stdlib.h>
#include <stdio.h>
#define N 26

int main(void)
{
	char *s;
	int i;

	s = malloc(N + 1);

	for(i = 0; i < N; i++)
		s[i] = 'a' + i;

	s[i] = '\0';

	printf("\"%s\"\n", s);

	return 0;
}
