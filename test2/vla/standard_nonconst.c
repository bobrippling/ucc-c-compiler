// RUN: %ocheck 1 %s '-DNULL=(void *)0' -fno-fold-const-vlas
// RUN: %ocheck 0 %s '-DNULL=0' -fno-fold-const-vlas
// RUN: %check %s '-DNULL=(void *)0' -pedantic

int null_is_ptr_type()
{
	char s[1][1+(int)NULL]; // CHECK: warning: cast-expr is a non-standard constant expression (for array size)
	int i = 0;
	sizeof(s[++i]);
	return i;
}

int another()
{
	int i = 1;
	sizeof(char[i++]);
	return i;
}

main()
{
	if(another() != 2)
		abort();

	return null_is_ptr_type();
}
