// RUN: %check -e %s

__attribute((visibility("hidden")))
void f(); // CHECK: note: previous declaration here (hidden)

__attribute((visibility("default")))
void f() // CHECK: error: visibility of "f" (default) does not match previous declaration
{
}

int g()
{
	f();
	return 3;
}
