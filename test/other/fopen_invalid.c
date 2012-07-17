#include <assert.h>
#include <stdio.h>
#include <errno.h>

int main()
{
	assert(!fopen(NULL, "rw") && errno == EINVAL);
}
