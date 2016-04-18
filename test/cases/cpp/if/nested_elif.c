// RUN: %ucc -E %s | grep 'is_64'
// RUN: %ucc -E %s | grep 'BAD'; [ $? -ne 0 ]

#if defined __x86_64__
# define __WORDSIZE  64
# define __WORDSIZE_COMPAT32  1
#else
# define __WORDSIZE  32
#endif

wordsize is __WORDSIZE

#if __WORDSIZE == 32

#  if A // test elif chosen is preserved
#  endif

  is_32

#elif __WORDSIZE == 64

#  if A // test elif chosen is preserved
#  endif

  is_64

#else

#  warning "unexpected value for __WORDSIZE macro"
   BAD

#endif
