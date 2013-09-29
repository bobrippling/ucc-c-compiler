#define str(...) #__VA_ARGS__
#define foo(a, b) foo a bar str(b)
#define bar foo bar 1
foo(bar, (1, 2, 3))
