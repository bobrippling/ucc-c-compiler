union
{
	int in;
	char *p;
	void (*f)();
	struct A
	{
		int i, j, k;
	};
} u = {
	1,2,3,4
};
