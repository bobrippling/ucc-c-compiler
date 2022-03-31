printf();

main()
{
	struct A {
		int x;
	};

	struct Flags {
		int flags;
	};

	struct C {
		struct A a, b, r;
		struct Flags *rkp;
	} c = {
		.a = { 1 },
		.b = { 3 },
	};
	struct C *pc = &c;

	pc->r = (pc->rkp && pc->rkp->flags) ? pc->a : pc->b;
	//       ^~~~~~~ we have some pointer stored as (&pc + N)
	//               we then do a non-zero check on pc->rkp,
	//               then spill (&pc + N), and in doing so eval `&pc + N`,
	//               which clobbers the CPU flags, meaning we go into the wrong branch
	//               and boom

	//printf("{ %d, %d }\n", pc->r.x, pc->r.y);
}
