#define str(s) # s
#define xstr(s) str(s)
#define debug(s, t) printf("x" # s "= %d, x" # t "= %s", \
x ## s, x ## t)
#define INCFILE(n) vers ## n
#define glue(a, b) a ## b
#define xglue(a, b) glue(a, b)
#define HIGHLOW "hello"
#define LOW LOW ", world"

debug(1, 2);
fputs(str(strncmp("abc\0d", "abc", '\4') // this goes away
== 0) str(: @\n), s);
#include xstr(INCFILE(2).h)
glue(HIGH, LOW);
xglue(HIGH, LOW)

// results in

printf("x" "1" "= %d, x" "2" "= %s", x1, x2);
fputs(
		"strncmp(\"abc\\0d\", \"abc\", '\\4') == 0" ": @\n",
		s);
#include "vers2.h" // (after macro replacement, before file access)
"hello";
"hello" ", world"

// or, after concatenation of the character string literals,

printf("x1= %d, x2= %s", x1, x2);
fputs(
		"strncmp(\"abc\\0d\", \"abc\", '\\4') == 0: @\n",
		s);
#include "vers2.h" // (after macro replacement, before ﬁle access)
"hello";
"hello, world"
