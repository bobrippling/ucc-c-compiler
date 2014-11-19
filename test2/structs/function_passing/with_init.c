// RUN: %ocheck 0 %s

typedef struct
{
	double x, y;
} Point;

typedef struct
{
	Point a, b;
} Both;

Point getpoint(double a, double b)
{
	return (Point){ .y = b, .x = a };
}

Both getboth()
{
	return (Both){ .b = getpoint(1, 2), .a = { 5, 6 } };
}

void assert(_Bool cond)
{
	if(!cond)
		abort();
}

main()
{
	Both b = getboth();

	assert(b.a.x == 5);
	assert(b.a.y == 6);
	assert(b.b.x == 1);
	assert(b.b.y == 2);

	return 0;
}
