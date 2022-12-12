#ifdef __UCC__

#  if 1 || __WORDSIZE == 64
#   define ULONG_MAX	18446744073709551615UL
#  else
#   define ULONG_MAX	4294967295UL
#  endif

unsigned long long strtoul(const char *, const char **, int);
int printf(const char *, ...);
enum { NULL };
typedef unsigned long size_t;

#else
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#endif

int nbdg(unsigned long n)
{
	switch (n) {
		case 1UL ... 9UL: return 1;
		case 10UL ... 99UL: return 2;
		case 100UL ... 999UL: return 3;
		case 1000UL ... 9999UL: return 4;
		case 10000UL ... 99999UL: return 5;
		case 100000UL ... 999999UL: return 6;
		case 1000000UL ... 9999999UL: return 7;
		case 10000000UL ... 99999999UL: return 8;
		case 100000000UL ... 999999999UL: return 9;
		case 1000000000UL ... 9999999999UL: return 10;
		case 10000000000UL ... 99999999999UL: return 11;
		case 100000000000UL ... 999999999999UL: return 12;
		case 1000000000000UL ... 9999999999999UL: return 13;
		case 10000000000000UL ... 99999999999999UL: return 14;
		case 100000000000000UL ... 999999999999999UL: return 15;
		case 1000000000000000UL ... 9999999999999999UL: return 16;
		case 10000000000000000UL ... 99999999999999999UL: return 17;
		case 100000000000000000UL ... 999999999999999999UL: return 18;
		case 1000000000000000000UL ... 9999999999999999999UL: return 19;
		case 10000000000000000000UL ... ULONG_MAX: return 20;
	}
	return 0;
}

int main(int argc, char **argv)
{
	if(argc > 1){
		unsigned long v = strtoul(argc > 1 ? argv[1] : "1111", NULL, 0);
		printf("%lu : %d\n", v, nbdg(v));
		return 0;
	}

	char buf[] = "12345678901234567890";

	for(size_t i = 1; i < sizeof(buf); i++){
		char save = buf[i];
		buf[i] = '\0';

		unsigned long v = strtoul(buf, NULL, 0);
		printf("%lu : %d\n", v, nbdg(v));

		buf[i] = save;
	}

	return 0;
}
