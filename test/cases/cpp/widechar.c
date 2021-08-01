// RUN: %ucc -E -P %s | grep pass

#if L'\0' == 0
pass
#endif
