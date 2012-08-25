struct A
{
        long a, b;
        int y, x;
};

typedef long unsigned size_t;

#define offsetof(type, member) ((size_t)&((type *)0)->member)

const int static i = offsetof(struct A, x);
