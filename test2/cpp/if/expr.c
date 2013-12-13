// RUN: %ucc -E %s | grep hello

#if 1 + 2 * 3 + 5 > 6 || 7 > 2
hello
#endif
