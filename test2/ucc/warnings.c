// RUN: %ucc -Wpaste -Wcpp -Wimplicit %s -'###' >%t 2>&1
// RUN: grep -F "ucc: unknown warning: '-Wcpp'" %t

// RUN: grep "cpp .*-Wpaste" %t
// RUN: ! grep "cpp .*-Wimplicit" %t

// RUN: grep "cc1 .*-Wimplicit" %t
// RUN: ! grep "cc1 .*-Wpaste" %t
