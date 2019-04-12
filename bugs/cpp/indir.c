#define FIRST(A, ...) A
#define REST(A, ...) __VA_ARGS__

#define HEADERS "stdio.h", "stdlib.h", "assert.h"

#define APPLY(f, ...) f(__VA_ARGS__)

#define IS_THE_WORD_FOO_CHECK_foo unused, 1
#define IS_THE_WORD_FOO_IMPL(UNUSED, RESULT, ...) RESULT
#define IS_THE_WORD_FOO(X) \
  APPLY(IS_THE_WORD_FOO_IMPL, IS_THE_WORD_FOO_CHECK_ ## X, 0, unused)

#define CHECK_EMPTY_PARENS_IMPL() something clever here
#define CHECK_EMPTY_PARENS(...) CHECK_EMPTY_PARENS_IMPL __VA_ARGS__


#define DOUBLE_SHIFT a

//#define RETURN_SECOND(a, b, ...) b
#define RETURN_SECOND(a, b, ...) return second, aa=a bb=b, va=__VA_ARGS__
#define TRUE_IF_TWO_ARGS(a, ...) APPLY(RETURN_SECOND, DOUBLE_SHIFT ## __VA_ARGS__, 0)


#define COUNT_(a, b, c, ...)  c
#define COUNT(...) COUNT_(__VA_ARGS__, 1, 0, 0)

#define IS_EMPTY_PARENS_IMPL() foo, dog
#define IS_EMPTY_PARENS(...) APPLY(COUNT, IS_EMPTY_PARENS_IMPL __VA_ARGS__)

IS_EMPTY_PARENS(abc)
IS_EMPTY_PARENS(())
IS_EMPTY_PARENS("timothy")


	/*
#define IS_EMPTY_PARENS(...) HELP_EMPTY_PARENS(__VA_ARGS__)

IS_EMPTY_PARENS(a) // 0
IS_EMPTY_PARENS(()) // 1
*/
