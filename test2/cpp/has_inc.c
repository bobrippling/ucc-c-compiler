// RUN: %ucc -E -o %t %s
// RUN: ! grep '^no' %t

#if !defined(__has_include)
#  error no __has_include
#endif

#if defined(__has_include) && __has_include(<stdlib.h>)
got stdlib (complex expr)
#else
no stdlib (complex expr)
#endif

#if __has_include(<stdio.h>)
got stdio
#else
no stdio
#endif

#define STDIO <stdio.h>
#if __has_include(STDIO)
got stdio (indirect)
#else
no stdio (indirect)
#endif

#if __has_include("has_inc.c")
got has_inc.c
#else
no has_inc.c
#endif

#define TIM "has_inc.c"
#if __has_include(TIM)
got has_inc.c (indirect)
#else
no has_inc.c (indirect)
#endif
