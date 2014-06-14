#ifndef CHAR_BIT
#    define CHAR_BIT 8
#endif
#define ARRAY_LEN(ar) (sizeof(ar) / sizeof *(ar))

#define SWAP(type, a, b) \
	do{                    \
		type tmp = (a);      \
		(a) = (b);           \
		(b) = tmp;           \
	}while(0)
