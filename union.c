union
{
	int x;
	struct
	{
		char c;
	};
	long long l;
	struct
	{
		int a, b, c;
	} z;
} u;

f()
{
	return u.z.b;
}
