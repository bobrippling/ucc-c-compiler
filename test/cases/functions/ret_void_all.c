// RUN: %check %s

// fnty retty
// void   int
//  int  void(expr)
//  int  void(nothing)
// void  void(expr)
// void  void(nothing)
//  int   int

void a()
{
	return 3; // CHECK: warning: return with a value in void function a
}

int b()
{
	return a(); // CHECK: warning: void return in non-void function b
}

int c()
{
	return; // CHECK: warning: empty return in non-void function c
}

void d()
{
	return a(); // CHECK: !/warn/
}

void e()
{
	return; // CHECK: !/warn/
}

int f()
{
	return 3; // CHECK: !/warn/
}
