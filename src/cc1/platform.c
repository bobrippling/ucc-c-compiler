#include <stdio.h>
#include <string.h>
#include <sys/utsname.h>

#include "platform.h"

static int init = 0, wordsize;

#define INIT() \
	do{ \
		if(!init){ \
			platform_init(); \
			init = 1; \
		} \
	}while(0)

static void platform_init()
{
	struct utsname u;

	if(uname(&u) == -1){
		perror("uname()");
		wordsize = 4;
	}else if(!strcmp("i686", u.machine)){
		wordsize = 4;
	}else if(!strcmp("x86_64", u.machine) || !strcmp("amd64", u.machine)){
		wordsize = 8;
	}else{
		fprintf(stderr, "unrecognised machine architecture: \"%s\"\n", u.machine);
		wordsize = 4;
	}
}

int platform_word_size()
{
	INIT();
	return wordsize;
}
