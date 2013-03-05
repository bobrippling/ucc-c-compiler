int a[6] = {
        [4] = 29,
        [2] = 15
};
int widths[] = {
        [0 ... 9] = 1,
        [10 ... 99] = 2,
        [100] = 3
};
struct point p = { .y = yvalue, .x = xvalue };
union foo f = { .d = 4 };
int a[6] = {
        [1] = v1,
        v2,
        [4] = v4
};
int whitespace[256] = {
        [' '] = 1,
        ['\t'] = 1,
        ['\h'] = 1,
        ['\f'] = 1,
        ['\n'] = 1,
        ['\r'] = 1
};
struct point ptarray[10] = {
        [2].y = yv2,
        [2].x = xv2,
        [0].x = xv0
};
