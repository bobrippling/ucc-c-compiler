#include <assert.h>

main()
{
	assert('\xa' == 10);
	assert('\11' ==  9);
	assert('\111' ==  73);
	assert("\xb"[0] == 11);
	assert(255 == 0xff);

	return 0;
}
