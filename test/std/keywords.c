// normal parsing
// RUN: %ucc %s -fsyntax-only -std=c89
// RUN: %ucc %s -fsyntax-only -std=c99
// RUN: %ucc %s -fsyntax-only -std=c11
//
// should reject gnu keywords
// RUN: %ucc %s -fsyntax-only -DEXT -std=c89; [ $? -ne 0 ]
// RUN: %ucc %s -fsyntax-only -DEXT -std=c99; [ $? -ne 0 ]
// RUN: %ucc %s -fsyntax-only -DEXT -std=c11; [ $? -ne 0 ]
//
// should reject C99 keywords in C89 mode
// RUN: %ucc %s -fsyntax-only -DSHOW_C99 -std=c89; [ $? -ne 0 ]
//
// should reject C99 keywords in gnu89 mode
// RUN: %ucc %s -fsyntax-only -DSHOW_C99 -std=gnu89; [ $? -ne 0 ]
//
// should accept gnu keywords with -fasm
// RUN: %ucc %s -fsyntax-only -DEXT -std=c89 -fasm
// RUN: %ucc %s -fsyntax-only -DEXT -std=c99 -fasm
// RUN: %ucc %s -fsyntax-only -DEXT -std=c11 -fasm
//
// should accept gnu keywords with -fgnu-keywords
// RUN: %ucc %s -fsyntax-only -DEXT -std=c89 -fgnu-keywords
// RUN: %ucc %s -fsyntax-only -DEXT -std=c99 -fgnu-keywords
// RUN: %ucc %s -fsyntax-only -DEXT -std=c11 -fgnu-keywords
//
// should accept gnu keywords with -std=gnu*
// RUN: %ucc %s -fsyntax-only -DEXT -std=gnu89
// RUN: %ucc %s -fsyntax-only -DEXT -std=gnu99
// RUN: %ucc %s -fsyntax-only -DEXT -std=gnu11


// should always work
__asm("");
__asm__("");
__typeof(0) i;
__typeof__(0) i;

f(char *__restrict p);
static __inline void g();

#if defined(EXT)
asm("");
typeof(0) i;
#endif

#if __STDC_VERSION__ >= 199901L || SHOW_C99

_Bool yo;

f(char *restrict p);

static inline void g()
{
}

#endif

//#if __STDC_VERSION__ >= 201112L
// should always handle C11 keywords - reserved namespace

_Noreturn void nr();
_Static_assert(1, "");
int ai = _Alignof(int);
_Alignas(long) int x;
int gen = _Generic(0, int: 2);

//#endif
