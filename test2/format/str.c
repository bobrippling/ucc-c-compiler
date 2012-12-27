f(char *, ...) __attribute__((format(printf, 1, 2)));

main()
{
	f("%s\n", "hi");
}
