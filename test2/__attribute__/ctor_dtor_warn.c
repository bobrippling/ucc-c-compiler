// RUN: %check %s

int x __attribute((constructor)); // CHECK: warning: constructor attribute on non-function

int main()
{
	void f() __attribute((constructor)); // CHECK: warning: constructor attribute on non-global function
}
