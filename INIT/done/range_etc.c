int a[6] = {
        [4] = 29,
        [2] = 15
};
int widths[] = {
        [0 ... 9] = 1,
        [10 ... 99] = 2,
        [100] = 3
};
struct point {int x, y; } p = { .y = 1, .x = 2 };
union foo { int d; } f = { .d = 4 };
int b[6] = {
#define v0 0
#define v1 1
#define v2 2
#define v4 4
        [1] = v1,
        v2,
        [4] = v4
};
int whitespace[256] = {
        [' '] = 1,
        ['\t'] = 1,
        //['\h'] = 1,
        ['\f'] = 1,
        ['\n'] = 1,
        ['\r'] = 1
};
struct point ptarray[10] = {
        [2].y = v2,
        [2].x = v2,
        [0].x = v0
};
