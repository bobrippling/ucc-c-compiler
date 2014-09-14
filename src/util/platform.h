#ifndef PLATFORM_H
#define PLATFORM_H

/* triple - cpu/arch, OS, vendor (don't care at the moment) */
enum platform_arch
{
	PLATFORM_MIPSEL,
	PLATFORM_x86
};

enum platform_os
{
	PLATFORM_LINUX,
	PLATFORM_FREEBSD,
	PLATFORM_DARWIN,
	PLATFORM_CYGWIN
};

enum platform_arch platform_arch(void);
enum platform_os platform_os( void);

#include "compiler.h"

unsigned platform_word_size(void) ucc_const;

unsigned platform_align_max(void) ucc_const;

void platform_set_word_size(unsigned);

/* no big endian support */
#define platform_bigendian() 0

#endif
