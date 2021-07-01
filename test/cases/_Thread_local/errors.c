// RUN: %check --only -e %s

_Thread_local int a;

void f()
{
	_Thread_local int b; // CHECK: error: static or extern required on non-global thread variable
	b = 3;
}

typedef _Thread_local int c; // CHECK: error: typedef has thread-local specified
