// RUN: %check %s -Wmissing-prototypes -Wmissing-variable-declarations

void f(void) // CHECK: warning: no previous prototype for non-static function
{
}

void g(); // CHECK: !/warn.*proto/
void g(void) // CHECK: warning: no previous prototype for non-static function
{
}

void h(void); // CHECK: !/warn.*proto/
void h(void) // CHECK: !/warn.*proto/
{
}

static void k(void) // CHECK: !/warn.*proto/
{
}

extern void l(void) // CHECK: warning: no previous prototype for non-static function
{
}

int a; // CHECK: warning: non-static declaration has no previous extern declaration

static int b; // CHECK: !/warn.*extern/

extern int c; // CHECK: !/warn.*extern/
