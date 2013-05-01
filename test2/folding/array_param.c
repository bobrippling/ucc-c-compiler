// RUN: %ucc -c %s; [ $? -ne 0 ]
// RUN: %ucc -c %s 2>&1 | %check %s

extern int f(int x[-1]); // CHECK: /error: negative array size/
