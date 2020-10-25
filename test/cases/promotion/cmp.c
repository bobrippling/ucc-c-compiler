// RUN: %ocheck 0 %s
void abort(void) __attribute__((noreturn));

main()
{
	signed   int a = -1;
	unsigned int b = 2;
	signed   short c = -1;
	unsigned short d = 2;

	/* 'a' is promoted to unsigned */
	if(a > b){
		// fine
	}else if(a < b){
		abort();
	}else{
		abort();
	}

	/* 'c' and 'd' fit in signed int,
	 * both promoted, keeping their sign */
	if(c > d){
		abort();
	}else if(c<d){
		// fine
	}else{
		abort();
	}

	return 0;
}
