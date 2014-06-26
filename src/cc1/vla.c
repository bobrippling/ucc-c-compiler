#include "../util/platform.h"

#include "vla.h"

unsigned vla_space()
{
	/*  T *ptr; size_t size;   */
	return platform_word_size() * 2;
}
