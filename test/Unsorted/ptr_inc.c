int main()
{
	int i = 5, j;
	int *p = &i;

	++*p;
	++(*p);
	(*p)++;

	j = ++i + 1;

	assert(j == 7);
	assert(p == &i + 3);
	assert(p == ((void *)&i) + 3 * sizeof(void *));

	return 0;
}
