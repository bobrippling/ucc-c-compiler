printf();

main()
{
	struct A {
		int x, y;
	};
	struct C {
		struct A a, b, r;
		struct {
			int flags;
		} *rkp;
	} c = {
		.a = { 1, 2 },
		.b = { 3, 4 },
	}, *pc = &c;

	int cond = 3;

#define FL_ISSET(l, f) ((l) & (f))
#define F_ISSET(p, f)  FL_ISSET((p)->flags, (f))
#define IS_MOTION(vp) ((vp)->rkp != 0 && F_ISSET((vp)->rkp, 5))

	pc->r = IS_MOTION(pc) ? pc->a : pc->b;

	printf("{ %d, %d }\n", pc->r.x, pc->r.y);
}
