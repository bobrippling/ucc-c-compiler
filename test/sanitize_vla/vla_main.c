#include "vla_oob.h"
#include "vla_sup.h"

extern int printf(const char *, ...)
	__attribute((format(printf,1,2)));

main()
{
	for(int i = 0; i < 12; i++){
		const size = g() + i, index = i * 2;

		printf("size=%d, i=%d. calling f(%d) oob=%d\n",
				size, index, i, index >= size);

		f(i);
	}
}
