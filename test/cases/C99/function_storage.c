// RUN: %check -e %s

auto int g(void); // CHECK: error: invalid storage class 'auto' on global scoped function

register int h(void); // CHECK: error: invalid storage class 'register' on global scoped function
