main()
{
	int write(int, void *, int);
	char *s = "hi\n";
	write(1, s, 3);
}
