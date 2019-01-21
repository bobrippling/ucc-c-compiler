// RUN: %ucc -E %s | grep good
// RUN: %ucc -E %s | grep bad; [ $? -ne 0 ]

#if 0
bad
#elif 0
bad
#elif 0
bad
#elif 1
good
#else
bad
#endif
