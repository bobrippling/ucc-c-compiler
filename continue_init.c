struct
{
	int i;
	int j;
	int k;
} x = {
	.j = 1,
	2, // should init k
	// 3,
};
