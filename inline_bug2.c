__attribute((always_inline))
inteq(int a, int b)
{
	return a == b;
}

__attribute((always_inline))
int *find(int *begin, int *end, int n, int cmp(int, int))
{
	int *i;
	for(i = begin; i != end; i++)
		if(cmp(*begin, n)) /* s/begin/i/ */
			break;
	return begin; /* s/begin/i/ */
}

int main()
{
	int ar[] = {
		1, 2, 3, 4, 5
	};
	int *arend = ar + 5;

	int *four = find(ar, arend, 4, inteq);

	if(four != &ar[3])
		printf(":(\n");

	int *end = find(ar, arend, 0, inteq);

	if(end != &ar[5])
		printf(":(\n");
}
