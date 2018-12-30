// RUN: %ocheck 0 %s

typedef unsigned char uint8_t;

assert(_Bool b)
{
	static unsigned i;
	if(!b){
		write(2, (char[]){ i + '0', '\n' }, 2);
		abort();
	}
	i++;
}

bad()
{
	uint8_t port = 0x23;

	_Static_assert(_Generic(~port, int: 1) == 1, "bad promotion");

	assert(~port == 0xffffffdc); // 2's complement
	assert(~port >> 4 == 0xfffffffd); // signed rshift
	assert((uint8_t)(~port >> 4) == 0xfd); // truncation
}

good()
{
	uint8_t port = 0x23;

	assert((uint8_t) ~port == 0xdc);
	assert((uint8_t) ~port >> 4 == 0x0d);
}

main()
{
	bad();
	good();

	return 0;
}
