// RUN: %ocheck 3 %s

typedef int func(void);

call(func ^p)
{
	return p();
}

main()
{
#include "../ocheck-init.c"
	return call(^{ return 3; });
}
