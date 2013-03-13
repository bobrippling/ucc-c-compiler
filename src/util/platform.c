#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>

#include "platform.h"

static int init = 0;
static enum platform     platform_t;
static enum platform_sys platform_s;

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
		exit(1);
	}else if(!strcmp("i686", u.machine)){
		platform_t = PLATFORM_32;
	}else if(!strcmp("x86_64", u.machine) || !strcmp("amd64", u.machine)){
		platform_t = PLATFORM_64;
	}else{
		fprintf(stderr, "unrecognised machine architecture: \"%s\"\n", u.machine);
		exit(1);
	}

	if(!strcmp(u.sysname, "Linux")){
		platform_s = PLATFORM_LINUX;
	}else if(!strcmp(u.sysname, "FreeBSD")){
		platform_s = PLATFORM_FREEBSD;
	}else if(!strcmp(u.sysname, "Darwin")){
		platform_s = PLATFORM_DARWIN;
	}else if(!strncmp(u.sysname, "CYGWIN_NT-", 10)){
		platform_s = PLATFORM_CYGWIN;
	}else{
		fprintf(stderr, "unrecognised machine system: \"%s\"\n", u.sysname);
		exit(1);
	}
}

unsigned platform_word_size()
{
	INIT();
	return platform_t == PLATFORM_32 ? 4 : 8;
}

enum platform platform_type()
{
	INIT();
	return platform_t;
}

enum platform_sys platform_sys()
{
	INIT();
	return platform_s;
}
