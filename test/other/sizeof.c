#ifndef __WORDSIZE
#  define __WORDSIZE 8
#endif

assert(int ln, _Bool b)
{
	if(!b){
		printf(__FILE__ ":%d\n", ln);
		abort();
	}
}
#define assert(x) assert(__LINE__, x)

main()
{
	char array[5];
	int i;
	void *p;
	int  *pi;
	char c;

	assert(sizeof array - 1 == 4);

	//assert(sizeof(void)   == 1);
	assert(sizeof(void *) == 8);
	//assert(sizeof(void *) == __WORDSIZE / 8);

	/*sizeof int;*/
	assert(sizeof(int) == 4);

	assert(sizeof i    == 4);
	assert(sizeof(i)   == 4);

	assert(sizeof p    == 8);
	assert(sizeof(p)   == 8);

	assert(sizeof pi   == 8);
	assert(sizeof(pi)  == 8);

	assert(sizeof c    == 1);
	assert(sizeof(c)   == 1);

	{
		typedef void *VP;
		void *p  = 5;
		VP    p2 = 5;

		//printf("sizeof(void *)=%d sizeof(void)=%d\n", sizeof(void *), sizeof(void));

		//printf("p=%d, p2=%d\n", p, p2);
		p++;
		p2++;
		//printf("p=%d, p2=%d\n", p, p2);

		assert(p  == 6);
		assert(p2 == 6);
	}
}
