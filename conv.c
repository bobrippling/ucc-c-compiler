extern char *strchr();

void f(int i)
{
	// should sub enough space for (char[n]), not (char *)
	char buf[] = "f(_)\n";

	*strchr(buf, '_') = i + '0';

	printf(buf);
}
