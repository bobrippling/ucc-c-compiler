// RUN: %ucc -E %s | grep noyo
// RUN: %check %s -E

#if 'abc' == 'cba' // CHECK: /warning: multi-char/
yo
#else
noyo
#endif
