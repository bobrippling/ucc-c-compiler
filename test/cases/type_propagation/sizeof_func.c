// RUN: %check %s

void *f();
typedef void V;
void (*get_f())();

a=sizeof f; // CHECK: warning: sizeof() on function type
b=sizeof(V); // CHECK: warning: sizeof() on void type
c=sizeof(void); // CHECK: warning: sizeof() on void type
d=sizeof *f(); // CHECK: warning: sizeof() on void type
e=sizeof *get_f(); // CHECK: warning: sizeof() on function type
