// RUN: %ucc %s
// RUN: %ucc %s -DDOG

// ensure defined DOG matches defined(DOG)
#if defined DOG
int a;
#else
char a;
#endif

// these #if blocks depend on the other (at link time)
#if defined(DOG)
extern int a;
main()
{
	return a;
}
#else
extern char a;
main()
{
	return a;
}
#endif
