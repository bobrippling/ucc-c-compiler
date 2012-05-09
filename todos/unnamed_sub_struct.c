typedef struct Point
{
	int x, y;
} Point;

typedef struct Size
{
	int w, h;
} Size;

typedef struct Rect
{
	Point; // unnamed - collapse into this struct
	Size;  // with -fms-extensions ?
} Rect;

main()
{
#ifdef __UCC__
#warning ucc
	Rect r;
	r.x = 0, r.y = 1, r.w = 2, r.h = 3;
#else
#warning gcc
	Rect r = { 0, 1, 2, 3 };
#endif

	r.w = 3;

	printf("{ { %d, %d }, { %d, %d }\n", r.x, r.y, r.w, r.h);
}
