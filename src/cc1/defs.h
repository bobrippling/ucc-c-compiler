#ifndef CHAR_BIT
#    define CHAR_BIT 8
#endif

#include "../util/macros.h"

#define SWAP(type, a, b) \
	do{                    \
		type tmp = (a);      \
		(a) = (b);           \
		(b) = tmp;           \
	}while(0)
