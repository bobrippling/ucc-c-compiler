#include <stdio.h>

//http://gcc.gnu.org/ml/gcc-help/2008-11/msg00084.html

#ifdef __UCC__
#  define types_compatible(a, b) _Generic((a)0,b:1, default:0)
#  define typeof __typeof
#else
#  define types_compatible(a, b) __builtin_types_compatible_p(a, b)
#endif

#define TEST(a,b) printf("%d <- %s, %s\n", types_compatible(a, b), #a, #b)

int main()
{
  char *char_ptr;
  const char *const_char_ptr;
  char char_arr[1];
  const char const_char_arr[1];

	printf("TODO: assert\n");

  TEST(char*, char*);
  TEST(char*, const char*);
  TEST(const char*, char*);
  //TEST(char[], char*);
  //TEST(char*, char[]);
  //TEST(char[], const char[]);
  TEST(typeof("hello"), char*);
  //TEST(typeof("hello"), char[]);
  TEST(typeof("hello"), const char*);
  //TEST(typeof("hello"), const char[]);
  TEST(typeof(&(*"hello")), char*);
  //TEST(typeof(&(*"hello")), char[]);
  TEST(typeof(&(*"hello")), const char*);
  //TEST(typeof(&(*"hello")), const char[]);
  TEST(typeof(*(&"hello")), char*);
  //TEST(typeof(*(&"hello")), char[]);
  TEST(typeof(*(&"hello")), const char*);
  //TEST(typeof(*(&"hello")), const char[]);
  TEST(typeof(&(("hello")[0])), char*);
  //TEST(typeof(&(("hello")[0])), char[]);
  TEST(typeof(&(("hello")[0])), const char*);
  //TEST(typeof(&(("hello")[0])), const char[]);

  TEST(typeof(&((char_ptr)[0])), char*);
  TEST(typeof(&((char_ptr)[0])), const char*);
  TEST(typeof(&((const_char_ptr)[0])), char*);
  TEST(typeof(&((const_char_ptr)[0])), const char*);
  TEST(typeof(&((char_arr)[0])), char*);
  TEST(typeof(&((char_arr)[0])), const char*);
  TEST(typeof(&((const_char_arr)[0])), char*);
  TEST(typeof(&((const_char_arr)[0])), const char*);
  return 0;
}
