#include <stdio.h>
 
void cpuid(unsigned info, unsigned *eax, unsigned *ebx, unsigned *ecx, unsigned *edx)
{
    __asm__(
        "cpuid;"                                            /* assembly code */
        :"=a" (*eax), "=b" (*ebx), "=c" (*ecx), "=d" (*edx) /* outputs */
        :"a" (info)                                         /* input: info into eax */
                                                            /* clobbers: none */
    );
}
 
int main()
{
  unsigned int eax, ebx, ecx, edx;
  int i;
 
  for (i = 0; i < 6; ++i)
  {
    cpuid(i, &eax, &ebx, &ecx, &edx);
    printf("eax=%i: %#010x %#010x %#010x %#010x\n", i, eax, ebx, ecx, edx);
  }
 
  return 0;
}
