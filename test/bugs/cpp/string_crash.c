#ifdef STDLIB
#include <assert.h>
#else
# define __ASSERT_VOID_CAST (void)
#  define assert(expr)							\
    ((expr)								\
     ? __ASSERT_VOID_CAST (0)						\
     : __assert_fail (#expr, __FILE__, __LINE__, __ASSERT_FUNCTION))
#endif

void test(void)
{
	assert(aaaaaaaaaaaaaaaaaa);
	assert(aaaaaaaaaaaaaaaaaaa);
	//     count_dirs       _
}
