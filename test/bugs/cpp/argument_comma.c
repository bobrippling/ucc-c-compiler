#define SECOND_ETC(x, ...) __VA_ARGS__

int x[2] = { SECOND_ETC(1'2,3'4) };
