#define OPEN  //{
#define CLOSE //}

main()
{
	int x[][2][3] = {
		OPEN
			OPEN 1, 2, 3 CLOSE,
			OPEN 1, 2, 3 CLOSE
		CLOSE,
		OPEN
			OPEN 1, 2, 3 CLOSE,
			OPEN 1, 2, 3 CLOSE
		CLOSE,
	};

	if(x[1][1][2] != 3)
		abort();

	return 0;
}
