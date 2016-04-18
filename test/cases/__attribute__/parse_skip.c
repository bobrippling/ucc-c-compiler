// RUN: %check %s

int __attribute__((unknown(char, abc(16)) )) a1;
int __attribute__((yo(z, w "", aligned(16)) )) a2;

__attribute((,,,))
void f(void);
__attribute((,))
void f(void);
__attribute(())
void f(void);
__attribute((, __ucc_debug)) // CHECK: warning: debug attribute handled
void f(void);
__attribute((__ucc_debug,)) // CHECK: warning: debug attribute handled
void f(void);
