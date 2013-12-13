// RUN: %check %s
// RUN: %layout_check %s

char auto_1[] = "hi"; // { h, i, 0 }
char zero_implicit[] = {};  // { 0 }
char uninit[2];
char padded[4] = "hi"; // { h, i, 0, 0 }
char exact[3] = "hi"; // { h, i, 0 }

// { h, i }
char no_nul[2] = "hi"; // CHECK: !/warning/

char overflow[2] = "hello"; // CHECK: /warning: string literal too long/
