// RUN: %ucc %s

typedef int fpos_t;

#define __unused __attribute__((unused))
#define NAM(x) x

int fa(void *fp __unused, char *s __unused, int n __unused)
{
}

void give_func(int (*give_func_arg)() __unused)
{
}

int *funopen2(
			const void   *NAM(pf) __unused,
			int         (*NAM(pa))(void *pa_arg_1, char       *pa_arg_2, int pa_arg_3) __unused,
			int         (*NAM(pb))(void *pb_arg_1, const char *pc_arg_3, int pb_arg_3) __unused,
			fpos_t      (*NAM(pc))(void *pc_arg_1, fpos_t      pc_arg_2, int pc_arg_3) __unused,
			int         (*NAM(pd))(void *pd_arg_1) __unused
		)
{
}

int fb(void *fp __unused, const char *s __unused, int n __unused)
{
}

fpos_t fc(void *fp __unused, fpos_t t __unused, int n __unused)
{
}

int fd(void *fp __unused)
{
}

main()
{
	void *p = (void *)5;


	funopen2(p, fa, fb, &fc, fd);

	return 0;
}
