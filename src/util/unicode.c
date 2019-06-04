#include "unicode.h"

enum char_type unicode_classify(const char *s, char **const end)
{
	switch(*s){
		case 'L':
			*end = (char *)s + 1;
			return UNICODE_L;

		case 'U':
			*end = (char *)s + 1;
			return UNICODE_U32;

		case 'u':
			if(s[1] == '8'){
				*end = (char *)s + 2;
				return UNICODE_u8;
			}

			*end = (char *)s + 1;
			return UNICODE_u16;
	}

	*end = (char *)s;
	return UNICODE_NO;
}
