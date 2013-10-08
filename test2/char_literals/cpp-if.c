// RUN: %ucc -E %s | grep hi

#if '\x14' == '\24' ==!! 'abcd'
hi
#else
yo
#endif
