#define JOIN_(a, b) a ## b
#define JOIN(a, b) JOIN_(a, b)

#define SYMBL(x) JOIN(__USER_LABEL_PREFIX__, x)

#if defined(__linux__)
#  define SECTION_NAME_TEXT .text
#  define SECTION_NAME_BSS .bss
#  define UCC_OS_LINUX
#elif defined(__DARWIN__) || defined(__MACH__)
#  define SECTION_NAME_TEXT __TEXT,__text
#  define SECTION_NAME_BSS __BSS,__bss
#  define UCC_OS_DARWIN
#else
#  error unknown target
#endif
