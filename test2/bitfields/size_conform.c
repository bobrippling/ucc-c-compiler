// RUN: %ucc %s

#include <stdio.h>
#include <limits.h>
#include <stdint.h>

#define SIZE_FMT "lu"

int main(void)
{
    struct sb  { _Bool    bf:1; };
    struct s8  { uint8_t  bf:1; };
    struct s16 { uint16_t bf:1; };
    struct s32 { uint32_t bf:1; };
    struct s64 { uint64_t bf:1; };

    _Static_assert(sizeof(struct sb)  == 1, "bad size");
    _Static_assert(sizeof(struct s8)  == 1, "bad size");
    _Static_assert(sizeof(struct s16) == 2, "bad size");
    _Static_assert(sizeof(struct s32) == 4, "bad size");
    _Static_assert(sizeof(struct s64) == 8, "bad size");
    return 0;
}
/*
sizeof (struct sb)  =  1 ( 8 bits)
sizeof (struct s8)  =  1 ( 8 bits)
sizeof (struct s16) =  2 (16 bits)
sizeof (struct s32) =  4 (32 bits)
sizeof (struct s64) =  8 (64 bits)
*/
