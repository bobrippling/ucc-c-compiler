static char *strchr(char *s, char c)
{
	while(*s)
		if(*s == c)
			return s;
	return (char *)0;
}

extern void printf(char *);

void f(int i)
{
	char buf[] = "f(_)\n";

	*strchr(buf, '_') = i + '0';

	printf(buf);
}
