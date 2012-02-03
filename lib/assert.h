#ifndef __ASSERT_H
#define __ASSERT_H


#ifndef NDEBUG
#  define assert(x) (x ? 0 : __assert_fail(#x, __LINE__, __FILE__, __func__))
void __assert_fail(const char *src, int line, const char *fname, const char *func);
#else
#  define assert(x)
#endif

#endif
