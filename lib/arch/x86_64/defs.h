#define JOIN_(a, b) a ## b
#define JOIN(a, b) JOIN_(a, b)

#define SYMBL(x) JOIN(__USER_LABEL_PREFIX__, x)

#if defined(__linux__)
#  define SECTION_NAME_TEXT .text
#  define SECTION_NAME_BSS .bss
#elif defined(__DARWIN__)
#  define SECTION_NAME_TEXT __TEXT,__text
#  define SECTION_NAME_BSS __BSS,__bss
#else
#  error unknown target
#endif
