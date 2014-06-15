/* TODO: signed char */
// RUN: %ucc -o %t %s
// RUN: %t | %output_check "size_t is 'unsigned long int'" "ptrdiff_t is 'long int'" "intmax_t is 'long long int'" "character constant is 'int'" "0x7FFFFFFF is 'int'" "0xFFFFFFFF is 'unsigned int'" "0x7FFFFFFFU is 'unsigned int'" "array of int is 'pointer to int'"

/* Get the name of a type */
#define typename(x) _Generic((x),                                                 \
        _Bool: "_Bool",                  unsigned char: "unsigned char",          \
         char: "char",         /*         signed char: "signed char",            */\
    short int: "short int",         unsigned short int: "unsigned short int",     \
          int: "int",                     unsigned int: "unsigned int",           \
     long int: "long int",           unsigned long int: "unsigned long int",      \
long long int: "long long int", unsigned long long int: "unsigned long long int", \
        float: "float",                         double: "double",                 \
  long double: "long double",                   char *: "pointer to char",        \
       void *: "pointer to void",                int *: "pointer to int",         \
      default: "other")

typedef long ptrdiff_t;
typedef unsigned long size_t;
typedef long long intmax_t;

void test_typename(void)
{
	size_t s;
	ptrdiff_t p;
	intmax_t i;
	int ai[3] = {0};

	printf("size_t is '%s'\n",             typename(s));
	printf("ptrdiff_t is '%s'\n",          typename(p));
	printf("intmax_t is '%s'\n",           typename(i));
	printf("character constant is '%s'\n", typename('0'));
	printf("0x7FFFFFFF is '%s'\n",         typename(0x7FFFFFFF));
	printf("0xFFFFFFFF is '%s'\n",         typename(0xFFFFFFFF));
	printf("0x7FFFFFFFU is '%s'\n",        typename(0x7FFFFFFFU));
	printf("array of int is '%s'\n",       typename(ai));
}

main()
{
	test_typename();
}
