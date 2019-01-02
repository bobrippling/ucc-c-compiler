#ifndef PLATFORM_H
#define PLATFORM_H

#include "compiler.h"
#include "triple.h"

enum arch platform_type(void);
enum sys platform_sys(void);
int platform_32bit(void);

unsigned platform_word_size(void) ucc_const;
unsigned platform_align_max(void) ucc_const;

/* no big endian support */
#define platform_bigendian() 0

void platform_init(enum arch, enum sys);

#endif
