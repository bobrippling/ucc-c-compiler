#ifndef __ASSERT_H
#define __ASSERT_H

#ifndef NDEBUG
#  define assert(x) (x ? 0 : __assert_fail(__LINE__, __FILE__))
// , __func__))

void __assert_fail(int line, const char *fname); //, const char *func);
#endif

#endif
