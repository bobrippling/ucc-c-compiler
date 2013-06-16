#define WIDTH 32
#define DIRECT(n) data##n
#define INDIRECT(n) DIRECT(n)
INDIRECT(WIDTH)
DIRECT(WIDTH)
