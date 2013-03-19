struct fred
{
	char s [6];
	int n;
};

struct fred x [] = {
	{
		{ "abc" },
		1
	},

	[0].s[0] = 'q'
};


struct fred y [] = {
	{
		{ "abc" },
		1
	},

	[0] = {
		.s[0] = 'q'
	}
};
