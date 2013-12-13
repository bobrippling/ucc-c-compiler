// RUN: %ucc -E %s | grep -F 'a AND b, c'
#define VA(x, ...) x AND __VA_ARGS__
VA(a, b, c)
VA(3)
