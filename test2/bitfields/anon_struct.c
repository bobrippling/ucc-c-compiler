// RUN: %ocheck 0 %s

struct bit
{
	int bit : 1;
};

struct bits
{
	struct bit; // `second' is not packed into this
	unsigned second : 1;
};

main()
{
	struct bits bs;

	bs.bit = 1;
	bs.second = 1;

	if(0[(int *)&bs] != 1)
		return 1;
	if(1[(int *)&bs] != 1) // ensure it's packed to an int spacing
		return 1;

	return 0;
}
