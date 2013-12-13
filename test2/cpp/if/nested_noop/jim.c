// RUN: %ucc -E %s | grep yo

#ifndef JIM_H
#define JIM_H

#include "yo.c"

#if X(2, 3)
yo
#endif

#endif
