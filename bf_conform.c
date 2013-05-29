#include <stdio.h>
#include <limits.h>
#include <stdint.h>

int main(void)
{
    struct sb  { _Bool    bf:1; };
    struct s8  { uint8_t  bf:1; };
    struct s16 { uint16_t bf:1; };
    struct s32 { uint32_t bf:1; };
    struct s64 { uint64_t bf:1; };

    printf("sizeof (struct sb)  = %2zu (%2zu bits)\n",
           sizeof (struct sb),
           sizeof (struct sb)  * CHAR_BIT);
    printf("sizeof (struct s8)  = %2zu (%2zu bits)\n",
           sizeof (struct s8),
           sizeof (struct s8)  * CHAR_BIT);
    printf("sizeof (struct s16) = %2zu (%2zu bits)\n",
           sizeof (struct s16),
           sizeof (struct s16) * CHAR_BIT);
    printf("sizeof (struct s32) = %2zu (%2zu bits)\n",
           sizeof (struct s32),
           sizeof (struct s32) * CHAR_BIT);
    printf("sizeof (struct s64) = %2zu (%2zu bits)\n",
           sizeof (struct s64),
           sizeof (struct s64) * CHAR_BIT);
    return 0;
}
/*
sizeof (struct sb)  =  1 ( 8 bits)
sizeof (struct s8)  =  1 ( 8 bits)
sizeof (struct s16) =  2 (16 bits)
sizeof (struct s32) =  4 (32 bits)
sizeof (struct s64) =  8 (64 bits)
*/
