union m
{
	// ordering here isn't important
	struct
	{
		char dir;
		char scan_ch;
	};
	int i;
};

union m m = {
	// designation here is important
	.dir = 1,
	.scan_ch = 2,
};
