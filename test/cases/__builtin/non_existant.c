// RUN: %check -e --only %s

void f()
{
	(void)__builtin_strlen("hi"); // CHECK: warning: implicit declaration of function "__builtin_strlen"
	(void)__builtin_doesntexist(1, 2, 3); // CHECK: error: unknown builtin '__builtin_doesntexist'
	// CHECK: ^warning: implicit declaration of function "__builtin_doesntexist"
}
