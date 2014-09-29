// RUN: %ocheck 5 %s

__attribute((noinline))
inline int a() // no code emitted yet
{
	return 2;
}
static int a(); // causes a to be emitted statically


__attribute((noinline))
inline int b() // no code emitted yet
{
	return 3;
}
extern int b(); // causes b to be emitted externally

main()
{
	return a() + b();
}
