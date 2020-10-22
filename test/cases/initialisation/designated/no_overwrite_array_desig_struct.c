// RUN: %ocheck 0 %s
// RUN: %layout_check %s

static const struct
{
	const char *date;
	int val;
	enum { GREY, GREEN, RED } colour;
	enum { CHANGE_NO, CHANGE_UP, CHANGE_DOWN } change;
} ent1[][3] = {
	[0][0] = { "3/1/13", 500, GREY,  CHANGE_NO,   },
	// [0][1] = {}
	// [0][2] = {}

	[1][0] = { "3/2/13", 600, GREEN, CHANGE_UP,   },
	[1][1] = { "3/6/13", 200, GREEN, CHANGE_UP,   },

	[2][0] = { "3/3/13", 400, RED,   CHANGE_DOWN, },
	[2][1] = { "3/4/13", 200, RED,   CHANGE_DOWN, },
	[2][2] = { "3/5/13", 100, RED,   CHANGE_DOWN, },
};

//__typeof(*data) p = &data;

f(int x, int y)
{
	__typeof(**ent1) *p = &ent1[x][y];

	return !!p->date;
}

main()
{
	return f(1, 2);
}
