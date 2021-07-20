// RUN: true
#define CALL(f, ...) f(__VA_ARGS__)
#define CONST()      123

CALL(CONST, ) // 123
