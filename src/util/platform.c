#include <stdio.h>
#include <string.h>
#include <sys/utsname.h>

#include "platform.h"

static int init = 0;
static enum platform platform_t;

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
		platform_t = PLATFORM_32;
	}else if(!strcmp("i686", u.machine)){
		platform_t = PLATFORM_32;
	}else if(!strcmp("x86_64", u.machine) || !strcmp("amd64", u.machine)){
		platform_t = PLATFORM_64;
	}else{
		fprintf(stderr, "unrecognised machine architecture: \"%s\"\n", u.machine);
		platform_t = PLATFORM_32;
	}
}

int platform_word_size()
{
	INIT();
	return platform_t == PLATFORM_32 ? 4 : 8;
}

enum platform platform_type()
{
	INIT();
	return platform_t;
}
