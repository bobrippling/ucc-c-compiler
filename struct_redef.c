// from https://lists.nongnu.org/archive/html/tinycc-devel/2021-01/msg00154.html

int f2_1 (void)
{
     struct foo { int a; };
     int i = sizeof(struct foo { int a, b; }); // gcc & clang mark only this as a duplicate
     for (i = sizeof(struct foo {int a, b, c;});;)
         return sizeof(struct foo { int a, b, c,d; }); // ...not this
}
