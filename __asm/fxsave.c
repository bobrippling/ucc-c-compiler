struct fxbuf {
	union {
		char bytes[512];
		struct {
			char res1[32 + 160 - 32];
			// 32
			//long double st[8]; // some other stuff interleaved
			// 160
			double xmm[16];
			char res2[3 * 16];
			char avail[3 * 16];
		};
	};
};
_Static_assert(sizeof(struct fxbuf) == 512, "");

void fxsave(struct fxbuf *p)
{
	__asm("fxsave %0 # 1=%1" : "=m"(*p) : "m"(*p));
}

double fx_xmm(struct fxbuf *p, int idx)
{
	return p->xmm[idx];
}

main()
{
	struct fxbuf buf;
	double d = 3;
	fxsave(&buf);

	printf("%f\n", fx_xmm(&buf, 0));
	printf("%f\n", fx_xmm(&buf, 1));
	printf("%f\n", fx_xmm(&buf, 2));
	//printf("%Lf\n", fx_st0(&buf, 2));
}
