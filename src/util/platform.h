#ifndef PLATFORM_H
#define PLATFORM_H

enum platform
{
	PLATFORM_32,
	PLATFORM_64
};

enum platform platform_type();

int platform_word_size();

#endif
