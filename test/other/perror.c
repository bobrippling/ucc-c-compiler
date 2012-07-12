#include <errno.h>

int main()
{
	perror(0);
	errno = ENOENT;
	perror("open()");
}
