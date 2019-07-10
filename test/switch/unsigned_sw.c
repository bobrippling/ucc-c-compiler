// RUN: %ocheck 0 %s
void abort(void) __attribute__((noreturn));

f(unsigned long long ull)
{
	switch(ull){
		case (signed char)-1:
			return 1;
		case 8:
			return 2;
	}
	return 3;
}

main()
{
	switch(7){
		case sizeof(int):
			;
	}

	if(f(-1) != 1)
		abort();

	return 0;
}
