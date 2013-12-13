// RUN: %ocheck 0 %s

main()
{
	char *s = "\12345";

	if(*s != 'S' || s[1] != '4' || s[2] != '5' || s[3])
		abort();

	return 0;
}
