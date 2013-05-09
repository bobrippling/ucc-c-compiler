// RUN: echo TODO; false

#include <sys/select.h>

main()
{
	struct timeval tv;
	fd_set a;

	tv.tv_sec  = 1;
	tv.tv_usec = 0;

	FD_ZERO(&a);
	FD_SET(0, &a);

	printf("%d\n", select(1, &a, 0, 0, &tv));
}
