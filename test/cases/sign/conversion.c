// RUN: %ocheck 0 %s
void abort(void) __attribute__((noreturn));

int main()
{
#include "../ocheck-init.c"
	unsigned x = (unsigned char)-5;

	if(x != 251)
		abort();

	return -5u < 10;
}
