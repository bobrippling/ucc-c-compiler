#if 0
#include <pthread.h>

static __thread int i = 3;

int printf();

void *t(void *c)
{
	printf("thread %d, i = %d, &i = %p\n", *(int *)c, i, &i);
	return 0;
}

int main()
{
	struct {
		pthread_t p;
		int i;
	} ts[4];

	for(int i = 0; i < sizeof(ts)/sizeof(*ts); i++){
		ts[i].i = i;
		pthread_create(&ts[i].p, 0, t, &ts[i].i);
	}

	for(int i = 0; i < sizeof(ts)/sizeof(*ts); i++){
		pthread_join(ts[i].p, 0);
	}
}
#else
static __thread int i = 3;
static __thread struct { int x, y; } s = { 3, 5 };
static __thread double d = 9.4;

int *p()
{
	return &i;
}
#endif
