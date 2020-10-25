// RUN: %ucc -E %s | grep hello

#if 9 == (1 + 2) * 3
hello
#endif
