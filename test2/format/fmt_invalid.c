f(int, int, char *, ...) __attribute__((format(printf, 3, 3)));
g(int, int, int, int, char *, ...) __attribute__((format(printf, 5, 4)));

main()
{
	f(0, 0, "hi", 2);
	g(0, 0, 0, 0, "hi", 2);
}
