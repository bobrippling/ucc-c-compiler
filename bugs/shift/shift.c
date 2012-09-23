#include <stdio.h>
#include <limits.h>

#define SH (CHAR_BIT * sizeof(unsigned long) / 2)

int main (void)
{
  unsigned long m;
  int sh = SH;

  printf ("SH = %d\n", (int) SH);
  m = 1UL << SH;
  printf ("m = 0x%lx\n", m);
  m = 1UL << sh;
  printf ("m = 0x%lx\n", m);
  m = 1UL << (SH/2);
  m *= m;
  printf ("m = 0x%lx\n", m);

  return 0;
}

#ifdef TIMMAY
gives:

SH = 32
m = 0x0
m = 0x1
m = 0x100000000

The correct result (as obtained with gcc) is:

SH = 32
m = 0x100000000
m = 0x100000000
m = 0x100000000
#endif
