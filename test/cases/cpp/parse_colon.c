// RUN: %check -e %s
#if 5 ? 2 // CHECK: /error: colon expected for ternary-\? operator/
#endif
