// RUN: %ocheck 3 %s

typedef int func(void);

call(func ^p)
{
	return p();
}

main()
{
	return call(^{ return 3; });
}
