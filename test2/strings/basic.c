// RUN: %ocheck 108 %s
// RUN: %output_check %s hi

f()
{
	char s[] = "hello";
	return s[2];
}

main()
{
	int write(int, void *, int);
	char *s = "hi\n";
	write(1, s, 3);

	return f();
}
