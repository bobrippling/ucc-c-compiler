// RUN: %ucc -E -o %t %s -nostdinc -isystem cpp/inc/
// RUN: ! grep '^no' %t

#if !defined(__has_include)
#  error no __has_include
#endif

/* Include files reference has/ under this file's
 * directory, so we're not dependant on the
 * environment for tests. */
#if defined(__has_include) && __has_include(<stdio.h>)
got stdio (complex expr)
#else
no stdio (complex expr)
#endif

#if __has_include(<stdio.h>)
got stdio (normal expr)
#else
no stdio (normal expr)
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
