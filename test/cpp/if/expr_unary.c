// RUN: %ucc -E %s | grep hi
// RUN: %ucc -E %s | grep bad; [ $? -ne 0 ]
#if !0
hi
#endif

#define A 1<<0
#define B 1<<1
#define C 1<<2

#if (A | C) & B
bad
#endif

