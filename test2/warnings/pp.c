#warning 1 // CHECK: /warning: +1/
hi
#ifdef A
#  warning 4 // CHECK: !/warning/
#else
#  warning 6 // CHECK: /warning: +6/
#endif
#warning 8 // CHECK: /warning: +8/
#pragma yo
#warning 10 // CHECK: /warning: +10/

// RUN: %check %s -E
