// RUN: %ocheck 0 %s -std=c11

struct bits
{
	struct
	{
		int bit : 1;
	}; // `second' is not packed into this

	unsigned second : 1;
};

void *memset(void *, int, unsigned long);

main()
{
	struct bits bs;

	memset(&bs, 0, sizeof bs);

	bs.bit = 1;
	bs.second = 1;

	if(0[(int *)&bs] != 1)
		return 1;
	if(1[(int *)&bs] != 1) // ensure it's packed to an int spacing
		return 2;

	return 0;
}
