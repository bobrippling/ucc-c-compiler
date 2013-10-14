// RUN: %ocheck 0 %s

main()
{
	char *s = "A\tX";

	if(s[0] != 'A' || s[1] != '\t' || s[2] != 'X' || s[3])
		abort();

	return 0;
}
