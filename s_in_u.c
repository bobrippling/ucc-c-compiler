union m
{
	int i;
	struct
	{
		char dir;
		char scan_ch;
	};
};

union m m = {
	.dir = -1,
	.scan_ch = 'a',
};
