assert(int ln, _Bool b)
{
	if(!b){
		printf("%d\n", ln);
		abort();
	}
}
#define assert(x) assert(__LINE__, x)

main()
{
	printf("\x1b[1;34mhi\033[mbeet\n");
	printf("hi\xa\xa\xa");

	assert(0xff == 255);
	assert('\xfa' == -6);
	assert("i\xfh"[1] == 15);

	assert(012 == 10);
	assert('\2' == 2);
	assert('\07' == 7);
	assert("\56yo"[0] == 46);

	assert(3 == 3);
	assert('3' == 3 + '0');
	assert(*"3" == 3 + '0');

	//assert(0b1011 == 11);
	//assert('\b10' == 2); - '\b' is already defined
	//assert("\b11011"[0] == 11);
}
