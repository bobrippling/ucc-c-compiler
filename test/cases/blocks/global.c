// RUN: %ocheck 0 %s

int x;

int (^b)(int) = ^(int i){
	x = 10;
	return i + 1;
};

int main()
{
#include "../ocheck-init.c"
	if(b(2) != 3){
		_Noreturn void abort();
		abort();
	}

	if(x != 10){
		_Noreturn void abort();
		abort();
	}

	return 0;
}
