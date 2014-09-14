// RUN: %check %s -std=c89 -Wvla

f(int x)
{
	int a[x]; // CHECK: warning: variable length array is a C99 feature
}
