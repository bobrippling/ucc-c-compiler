#define assert(x) if(!(x)){printf("assertion %s failed\n", #x); return 1;}
main()
{
	assert('\xa' == 10);
	assert('\11' ==  9);
	assert('\111' ==  73);
	assert("\xb"[0] == 11);
	0xff;

	return 0;
}
