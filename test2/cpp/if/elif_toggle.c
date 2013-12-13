// RUN: %ucc -E %s | grep good
// RUN: %ucc -E %s | grep bad; [ $? -ne 0 ]

#define YO

#ifdef YO
good
#elif 1
bad
#else
bad
#endif
