#ifndef RETAIN_H
#define RETAIN_H

typedef void retain_free_f(void *);

struct retain
{
	unsigned retains;
	retain_free_f *pfree;
};

#define RETAIN(x) (x ? (++((x)->rc).retains) : 0, (x))

#define RELEASE(x)                   \
	do{                                \
		if(x && --(x)->rc.retains == 0)  \
			(x)->rc.pfree(x);              \
	}while(0)                          \

#define RETAIN_INIT(x, pf) \
	(x)->rc.retains = 1,     \
	(x)->rc.pfree = (retain_free_f *)(pf)

#endif
