// RUN: %ucc -E %s | grep good
// RUN: %ucc -E %s | grep bad; [ $? -ne 0 ]

#define yo

#if defined(yo) << 1
good
#endif

#if hello * 3
bad
#endif
