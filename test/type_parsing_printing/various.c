// RUN: %ucc -fsyntax-only %s

int atexit_b(void (^)(void));
int atoi(const char *);
int (*(*pf_pf)())();
int read(void *p);
int (*rf)();
void _exit(int) __attribute__((__noreturn__));
void f(void *);
void g(void (*pf)());
void (*pf)();
void (*(*pfpf)())();
void tpf(void (*pfv)(void));
