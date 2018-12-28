// RUN: %check %s

static void f(void); // CHECK: warning: function declared static but not defined
static void g(void); // CHECK: !/warn/

void g()
{
}
