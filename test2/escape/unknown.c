// RUN: %ocheck 0 %s

main()
{
	char *s = "\890";
	if(s[0] != '8' || s[1] != '9' || s[2] != '0' || s[3])
		abort();
	return 0;
}
