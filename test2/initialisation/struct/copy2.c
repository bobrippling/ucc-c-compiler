// RUN: %ucc -o %t %s
// RUN: %t | %output_check '{ 1 2, 0, 0 }'

struct Pt
{
	int x, y;
};

struct Sz
{
	int w, h;
};

struct Rect
{
	struct Pt pt;
	struct Sz sz;
};

f(struct Rect *r)
{
	printf("{ %d %d, %d %d }\n",
			r->pt.x, r->pt.y,
			r->sz.w, r->sz.h);
}

g(struct Pt *pt)
{
	f( &(struct Rect){ *pt });
}

main()
{
	g((struct Pt[]){ 1, 2 });
}
