#include <stdio.h>
#include <stddef.h>

struct foo1 { char s[80]; };
struct foo2 { char s[80]; }  __attribute__ ((aligned (64)));
struct bar1 { struct foo1 f; int i; };
struct bar2 { struct foo2 f; int i; };
#define P(arg) printf("sizeof(" #arg ") = %u\n", (unsigned)sizeof(arg))

int main(void)
{
  P(struct foo1);
  P(struct foo2);
  P(struct bar1); printf("offset=%u\n", (unsigned)offsetof(struct bar1, i));
  P(struct bar2); printf("offset=%u\n", (unsigned)offsetof(struct bar2, i));
  return 0;
}
