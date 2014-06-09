extern void *malloc(int), *memcpy(void *, const void *, int);

f(int *p)
{
	int x[] = { 5, 2 };

	memcpy(p, x, sizeof x);

	//int *y = malloc(2 * sizeof(int));
	//memcpy(p, y, 2 * sizeof y);

	return 0;
}
