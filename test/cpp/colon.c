// RUN: %ucc -E %s | grep yo > /dev/null
#if 5 > 2 ? 5 ? 2 : 1 - 3 : 2 - 3
yo
#endif
