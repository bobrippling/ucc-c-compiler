void *malloc(unsigned long);
void free(void *);

main()
{
	int *p = malloc(10*sizeof*p);

	for(int i = 0; i < 10; i++)
		p[i] = 10 - i;

	free(p);
}
