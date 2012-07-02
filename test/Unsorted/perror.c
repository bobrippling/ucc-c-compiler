#include <errno.h>

int main()
{
	perror(0);
	perror("open()");
}
