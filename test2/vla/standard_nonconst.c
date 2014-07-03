// RUN: %ocheck 1 %s '-DNULL=(void *)0'
// RUN: %ocheck 0 %s '-DNULL=0'

int null_is_ptr_type()
{
	char s[1][1+(int)NULL];
	int i = 0;
	sizeof s[i++];
	return i;
}

main()
{
	return null_is_ptr_type();
}
