// RUN: %ucc -o %t %s
// RUN: %ocheck 108 %t
// RUN: %t | grep -F hi

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
