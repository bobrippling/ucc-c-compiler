// RUN: %layout_check %s
struct fred
{
	char s[6];
	int n;
};

struct fred ent1[] = {
	{
		{ "abc" },
		1
	},

	[0].s[0] = 'q'
};


struct fred ent2[] = {
	{
		{ "abc" },
		1
	},

	[0] = {
		.s[0] = 'q'
	}
};
