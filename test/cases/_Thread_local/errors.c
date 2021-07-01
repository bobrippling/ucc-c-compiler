// RUN: %check -e --only          %s -target x86_64-linux
// RUN: %check -e --prefix=darwin %s -target x86_64-darwin

#ifdef __DARWIN__
#  ifndef __STDC_NO_THREADS__
#    error expected __STDC_NO_THREADS__ to be defined for this target
#  endif
#else
#  ifdef __STDC_NO_THREADS__
#    error expected __STDC_NO_THREADS__ to not be defined for this target
#  endif
#endif

_Thread_local int a; // CHECK-darwin: thread-local storage is unsupported on this target

_Thread_local void f() // CHECK: error: _Thread_local on non-variable
{
	_Thread_local int b; // CHECK: error: static or extern required on non-global thread variable
	b = 3;
}

typedef _Thread_local int c; // CHECK: error: typedef has thread-local specified
