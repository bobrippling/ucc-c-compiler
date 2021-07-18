// RUN: %layout_check %s

struct A
{
	struct B
	{
		struct C
		{
			int j, k;
		} c;
	} b;
} a = {
	.b.c = {
		.j = 1,
		.k = 3
	}
};

struct A b = {
	.b.c = {
		.k = 5,
		.j = 6
	}
};

struct X
{
	struct Y
	{
		int j, k;
	} sub;
} as[] = {
	[0].sub = {
		.j = 1,
		.k = 3
	}
};

struct X bs[] = {
	[1].sub = {
		.k = 5,
		.j = 6
	}
};
