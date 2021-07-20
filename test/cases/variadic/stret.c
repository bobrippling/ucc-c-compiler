// RUN: true
//#include <stdio.h>
//#include <stdarg.h>

extern printf();

#define va_list __builtin_va_list
#define va_start __builtin_va_start
#define va_end __builtin_va_end
#define va_arg __builtin_va_arg

struct test {
    void *foo, *bar, *baz;
};

struct test example(int a, int b, ...)
{
    va_list va;
    va_start(va, b);
    printf("a: %d, b: %d, va_arg: %d\n", a, b, va_arg(va, int));
    va_end(va);
    return (struct test) {};
}

int main()
{
    example(1, 2, 3);
    return 0;
}
