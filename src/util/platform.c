#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>

#include "platform.h"

static int init = 0;
static const enum platform platform_t = UCC_PLATFORM;
static enum platform_sys platform_s; /* TODO: sys is decided at compile time */

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
	switch(platform_t){
		case PLATFORM_armv7:
		case PLATFORM_mipsel_32:
		 return 4;
		case PLATFORM_x86_64:
		 return 8;
	}
	abort();
}

int platform_32bit(void)
{
	return platform_word_size() == 4;
}

unsigned platform_align_max()
{
	switch(platform_word_size()){
		case 4:
			return 8;
		case 8:
			return 16;
	}
	abort();
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
