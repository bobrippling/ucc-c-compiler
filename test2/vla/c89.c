// RUN: %check %s -std=c89

f(int x)
{
	int a[x]; // CHECK: warning: variable length array is a C99 feature
}
