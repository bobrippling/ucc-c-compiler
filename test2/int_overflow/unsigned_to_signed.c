// RUN: %check %s

static const int INT_MAX = -1U / 2; // CHECK: warning: implicit cast changes value from 2147483648 to -2147483648
