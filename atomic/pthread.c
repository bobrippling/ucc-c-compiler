#include <stdio.h>
#include <pthread.h>

_Atomic int atom = 0 ;

f(void)
{
	int a = atom++;

	char buf[32];
	int n = snprintf(buf, sizeof buf, "%d\n", a);

	write(1, buf, n);
}

void *dispatch(void *unused)
{
	f();
	return 0;
}

main()
{
	pthread_t threads[10];

	for(int i = 0; i < 10; i++){
		pthread_create(&threads[i], NULL, dispatch, NULL);
	}

	for(int i = 0; i < 10; i++){
		pthread_join(threads[i], NULL);
	}
}
