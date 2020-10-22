// RUN: %ocheck 0 %s

#define is_int_good(ty) (ty)1.1 == 1
#define is_int_bad(ty)  (ty)0.1 == 0

main()
{
	if(!is_int_good(_Bool)) return 1;
	if(!is_int_good(char)) return 2;
	if(!is_int_good(short)) return 3;
	if(!is_int_good(int)) return 4;
	if(!is_int_good(long)) return 5;
	if(!is_int_good(long long)) return 6;

	if(is_int_good(float)) return 7;
	if(is_int_good(double)) return 8;
	if(is_int_good(long double)) return 9;

	if( is_int_bad(_Bool)) return 10; /* SPECIAL */
	if(!is_int_bad(char)) return 11;
	if(!is_int_bad(short)) return 12;
	if(!is_int_bad(int)) return 13;
	if(!is_int_bad(long)) return 14;
	if(!is_int_bad(long long)) return 15;

	if(is_int_bad(float)) return 16;
	if(is_int_bad(double)) return 17;
	if(is_int_bad(long double)) return 18;

	return 0;
}
