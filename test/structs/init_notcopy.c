// RUN: %check %s
// RUN: %ocheck 0 %s

typedef struct
{
	double x, y;
} Point;

typedef struct
{
	Point a, b;
} Both;

int geti()
{
	return 3;
}

Both getboth()
{
	return (Both){ .b = geti(), .a = { 5, 6 } }; // CHECK: /warning: 1 missing initialiser for 'struct <.*>'/
	//                  ^ initialises .b.x, not .b
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
	assert(b.b.x == 3);
	assert(b.b.y == 0);

	return 0;
}
