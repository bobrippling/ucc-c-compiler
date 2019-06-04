#include <string.h>

#include "std.h"

int std_from_str(const char *std, enum c_std *penu, int *gnu)
{
	if(!strcmp(std, "-ansi"))
		goto std_c90;

	if(strncmp(std, "-std=", 5))
		return 1;
	std += 5;

	if(!strncmp(std, "gnu", 3)){
		if(gnu)
			*gnu = 1;
		std += 3;
	}else if(*std == 'c'){
		if(gnu)
			*gnu = 0;
		std++;
	}else{
		return 1;
	}

	if(!strcmp(std, "99")){
		*penu = STD_C99;
	}else if(!strcmp(std, "90")){
std_c90:
		*penu = STD_C90;
	}else if(!strcmp(std, "89")){
		*penu = STD_C89;
	}else if(!strcmp(std, "11")){
		*penu = STD_C11;
	}else if(!strcmp(std, "17") || !strcmp(std, "18")){
		*penu = STD_C18;
	}else{
		return 1;
	}

	return 0;
}
