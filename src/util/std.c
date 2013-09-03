#include <string.h>

#include "std.h"

int std_from_str(const char *std, enum c_std *penu)
{
	if(!strcmp(std, "-ansi"))
		goto std_c90;

	if(strncmp(std, "-std=", 5))
		return 1;
	std += 5;

	if(!strcmp(std, "c99")){
		*penu = STD_C99;
	}else if(!strcmp(std, "c90")){
std_c90:
		*penu = STD_C90;
	}else if(!strcmp(std, "c89")){
		*penu = STD_C89;
	}else if(!strcmp(std, "c11")){
		*penu = STD_C11;
	}else{
		return 1;
	}

	return 0;
}
