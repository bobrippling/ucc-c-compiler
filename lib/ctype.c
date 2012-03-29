#include "ctype.h"
#include "stdlib.h"
#include "ucc_attr.h"

// must be functions, _can_ also be macros

int isalnum( int c){ return isalpha(c) || isdigit(c); }
int isalpha( int c){ return isupper(c) || islower(c); }
int isascii( int c){ return !(c & 0x7fff); } // ???
int isblank( int c){ return c == ' ' || c == '\t'; }
int isdigit( int c){ return '0' <= c && c <= '9'; }
int islower( int c){ return 'a' <= c && c <= 'z'; }
int isupper( int c){ return 'A' <= c && c <= 'Z'; }

int iscntrl( int c __unused){ abort(); }
int isgraph( int c __unused){ abort(); }
int isprint( int c __unused){ abort(); }
int ispunct( int c __unused){ abort(); }

int isxdigit(int c)
{
	if(isdigit(c))
		return 1;
	switch(c){
		case 'a' ... 'f':
		case 'A' ... 'F':
			return 1;
	}
	return 0;
}

int isspace(int c)
{
	if(isblank(c))
		return 1;

	switch(c){
		case '\f':
		case '\n':
		case '\r':
		case '\t':
		case '\v':
			return 1;
	}
	return 0;
}
