struct A
{
        long a, b;
        int y, x;
};

#ifdef TEST_OO
#define offsetof(type, member) ((int)&((type *)0)->member)
const int static i = offsetof(struct A, x);
#endif

struct A a;
int *p = &a.x;
