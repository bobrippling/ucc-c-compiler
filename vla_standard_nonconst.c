int null_is_ptr_type()
{
	char s[1][1+(int)NULL];
	int i = 0;
	sizeof s[i++];
	return i;
}
