#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/utsname.h>

#include "platform.h"

static enum arch arch;
static enum sys sys;
static int init;

unsigned platform_word_size()
{
	assert(init);
	switch(arch){
		case ARCH_i386: return 4;
		case ARCH_x86_64: return 8;
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

enum arch platform_type()
{
	assert(init);
	return arch;
}

enum sys platform_sys()
{
	assert(init);
	return sys;
}

void platform_init(enum arch a, enum sys s)
{
	arch = a;
	sys = s;
	init = 1;
}
