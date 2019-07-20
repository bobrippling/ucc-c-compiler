// RUN: %ucc -fsyntax-only %s
int printf(const char *, ...) __attribute__((format(printf, 1, 2)));

extern void v;

main()
{
	printf("%p\n", &v);
}
