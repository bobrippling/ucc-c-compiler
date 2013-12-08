#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>

#include "platform.h"

/* TODO: sys is decided at compile time */

static const enum platform_arch arch = UCC_PLATFORM;
static enum platform_os os;

static unsigned word_size;


#define INIT()         \
	do{                  \
		if(!word_size)     \
			platform_init(); \
	}while(0)

static void platform_init()
{
	struct utsname u;

	if(uname(&u) == -1){
		perror("uname()");
		exit(1);
	}

	if(!strcmp(u.sysname, "Linux")){
		os = PLATFORM_LINUX;
	}else if(!strcmp(u.sysname, "FreeBSD")){
		os = PLATFORM_FREEBSD;
	}else if(!strcmp(u.sysname, "Darwin")){
		os = PLATFORM_DARWIN;
	}else if(!strncmp(u.sysname, "CYGWIN_NT-", 10)){
		os = PLATFORM_CYGWIN;
	}else{
		fprintf(stderr, "unrecognised machine system: \"%s\"\n", u.sysname);
		exit(1);
	}

	if(strstr(u.machine, "64"))
		word_size = 8;
	else
		word_size = 4;
}

unsigned platform_word_size()
{
	INIT();
	return word_size;
}

void platform_set_word_size(unsigned sz)
{
	word_size = sz;
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

enum platform_arch platform_arch()
{
	INIT();
	return arch;
}

enum platform_os platform_os()
{
	INIT();
	return os;
}
