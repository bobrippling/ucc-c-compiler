//#include <stdio.h>
int printf();
#include <pthread.h>

extern int p(void);

void *routine(void *a)
{
	(void)a;

	printf("routine, p() = %d\n", p());

	return 0;
}

int main()
{
	printf("main, p() = %d\n", p());

	pthread_t c;
	pthread_create(&c, NULL, &routine, NULL);
}
