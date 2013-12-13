// RUN: %ocheck 0 %s

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
	if(r->pt.x != 1 || r->pt.y != 2
	|| r->sz.h || r->sz.w)
		abort();
}

g(struct Pt *pt)
{
	f( &(struct Rect){ *pt });
}

main()
{
	g((struct Pt[]){ 1, 2 });
	return 0;
}
