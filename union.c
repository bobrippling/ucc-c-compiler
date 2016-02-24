union
{
	int i;
	struct
	{
		unsigned a : 3;
		unsigned b : 4;
	} bfs;
	struct
	{
		char c; // TODO: access this
	};
	struct
	{
		char c;
	} named;
	long long l;
	struct
	{
		int x, y, z;
	} z;
} u = { .bfs = { 2, 3 } };

f()
{
	u.c = 7;
	return u.z.y;
}
