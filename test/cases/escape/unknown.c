// RUN: %check -e %s

main()
{
	char *s = "\890"; // CHECK: error: invalid escape character
	return 0;
}
