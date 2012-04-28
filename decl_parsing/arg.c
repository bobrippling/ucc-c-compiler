typedef int fpos_t;

#define NAM(x) x

int fa(void *fp, char *s, int n)
{
}

void give_func(int (*give_func_arg)())
{
}

int *funopen(
			const void   *NAM(pf),
			int         (*NAM(pa))(void *pa_arg_1, char       *pa_arg_2, int pa_arg_3),
			int         (*NAM(pb))(void *pb_arg_1, const char *pc_arg_3, int pb_arg_3),
			fpos_t      (*NAM(pc))(void *pc_arg_1, fpos_t      pc_arg_2, int pc_arg_3),
			int         (*NAM(pd))(void *pd_arg_1)
		)
{
}

int fb(void *fp, const char *s, int n)
{
}

fpos_t fc(void *fp, fpos_t t, int n)
{
}

int fd(void *fp)
{
}

main()
{
	void *p = (void *)5;


	funopen(p, fa, fb, &fc, fd);
}
