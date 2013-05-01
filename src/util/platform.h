#ifndef PLATFORM_H
#define PLATFORM_H

enum platform
{
	PLATFORM_mipsel_32,
	PLATFORM_x86_64
};

enum platform_sys
{
	PLATFORM_LINUX,
	PLATFORM_FREEBSD,
	PLATFORM_DARWIN,
	PLATFORM_CYGWIN
};

enum platform     platform_type(void);
enum platform_sys platform_sys( void);

int platform_word_size(void);

#endif
