// RUN: %ucc -o %t %s && %t hello | grep 'argv\[1\] = "hello"'
int printf(const char *, ...) __attribute__((format(printf, 1, 2)));

main(int argc, char **argv)
{
	for(__typeof(0) i = 0; i < argc; i++)
		printf("argv[%d] = \"%s\"\n", i, argv[i]);
	return 0;
}
