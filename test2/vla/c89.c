// RUN: %check --prefix=c89 %s -std=c89 -Wc89-vla
// RUN: %check --prefix=vla %s -Wvla

f(int x)
{
	int a[x]; // CHECK-c89: warning: variable length array is a C99 feature
	// CHECK-vla: ^ warning: use of variable length array
}
