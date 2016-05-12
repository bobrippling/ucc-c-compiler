// RUN: %ucc -fsyntax-only %s

typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;
typedef unsigned long long uint64_t;

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
