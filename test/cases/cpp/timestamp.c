// RUN: %ucc -fsyntax-only %s

int printf(const char *, ...) __attribute__((format(printf, 1, 2)));

main()
{
	printf("%s\n", __TIMESTAMP__);
}
