main()
{
	int printf(const char *, ...) __attribute((format(printf, 1, 2)));

	printf("%*.*s\n", 5, 3, "abcdefghijklmno");
}
