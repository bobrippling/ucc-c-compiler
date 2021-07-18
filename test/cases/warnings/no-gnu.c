// RUN: %check %s -Wno-gnu

// passing -Wno-gnu shouldn't enable warnings about gnu extensions

__attribute__(()) int x; // CHECK: !/warning: use of GNU __attribute__/
