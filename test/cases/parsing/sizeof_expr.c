// RUN: %ocheck 0 %s
void abort(void) __attribute__((noreturn));

typedef struct
{
	int x;
} A;

main()
{
	int sz = sizeof((A *)0)->x;
	if(sz != sizeof(int))
		abort();

	int ar[10];
	sz = sizeof(ar)[0];
	if(sz != sizeof(int))
		abort();

	return 0;
}
