// RUN: %ocheck 0 %s
void abort(void) __attribute__((noreturn));

char *s = "";
char b = '';

main()
{
	if(b != 22)
		abort();
	if(s[0] != 22)
		abort();
	if(s[1])
		abort();

	return 0;
}
