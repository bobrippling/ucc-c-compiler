#include <stdio.h>

void cpuid(unsigned info, unsigned *eax, unsigned *ebx, unsigned *ecx, unsigned *edx)
{
    __asm__(
				"cpuid"
        : "=a" (*eax)
				, "=b" (*ebx)
				, "=c" (*ecx)
				, "=d" (*edx)
				: "a" (info));
}

int main()
{
  for(int i = 0; i < 6; ++i){
		unsigned int eax, ebx, ecx, edx;
    cpuid(i, &eax, &ebx, &ecx, &edx);

    printf("eax=%i: %#010x %#010x %#010x %#010x\n", i, eax, ebx, ecx, edx);
  }

  return 0;
}
