// RUN: %ucc -c %s

struct A
{
        long a, b;
        int y, x;
};

#define offsetof(type, member) ((int)&((type *)0)->member)
const int static i = offsetof(struct A, x);

struct A a;
int *p = &a.x;
