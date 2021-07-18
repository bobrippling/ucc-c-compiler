// RUN: %ucc -fsyntax-only -fno-show-line -Wtenative-init -x cpp-output %s >%t 2>&1
// RUN: grep '^pop\.c:10:26: warning: default-init.*platform_s' %t
// RUN: test $(grep --count . %t) -eq 1

# 1 "pop.c"
# 1 "<built-in>"
# 1 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 1 "<command-line>" 2
# 1 "pop.c"

# 5 "pop.c" 2

# 1 "pop.h" 1




# 4 "pop.h"
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

enum platform platform_type(void);
enum platform_sys platform_sys( void);
int platform_32bit(void);

# 1 "compiler.h" 1
# 23 "pop.h" 2

unsigned platform_word_size(void) ;

unsigned platform_align_max(void) ;




const char *platform_name(void) ;
# 7 "pop.c" 2

static int init = 0;
static const enum platform platform_t = PLATFORM_x86_64;
static enum platform_sys platform_s;
