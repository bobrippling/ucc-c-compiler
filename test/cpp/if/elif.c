// RUN: %ucc -E %s | grep yo
// RUN: %ucc -E %s | grep bad; [ $? -ne 0 ]

#define A 1

#if !1
bad
#elif A > B+C
yo
#else
bad
#endif
