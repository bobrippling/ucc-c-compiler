int i = 5;
int j = 6;

int *ptrs[] = {
	&i,
	&j
};

main()
{
#define P(x) &x, x

	printf("i={%p,%d} j={%p,%d} ptrs[0]={%p,%d} ptrs[1]={%p,%d}\n",
			P(i), P(j),
			ptrs[0], *ptrs[0],
			ptrs[1], *ptrs[1]);
}
