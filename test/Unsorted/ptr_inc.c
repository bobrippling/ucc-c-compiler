int main()
{
	int i = 5, j;
	int *p = &i;

	++*p;
	++(*p);
	(*p)++;

	j = ++i + 1;

	return i;
}
