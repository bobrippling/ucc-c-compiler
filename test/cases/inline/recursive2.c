// RUN: %ocheck 7 %s -finline-functions
void abort(void) __attribute__((noreturn));

int count;

int r(int x)
{
	if(x > 0){
		count += x;
		return r(x - 1);
	}
	return 7;
}

int main()
{
#include "../ocheck-init.c"
	int ret = r(3);

	if(count != 6)
		abort();

	return ret;
}
