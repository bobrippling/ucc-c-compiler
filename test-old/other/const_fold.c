#include <assert.h>
main()
{
	const int i = 7;//8;
	int b = 0;

	switch(i){
		case 5 + 2:
			b = 1;
			break;

		case sizeof(int):
			;
	}

	assert(b);
}
