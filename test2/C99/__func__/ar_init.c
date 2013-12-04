// RUN: %check %s

main()
{
	static char x[] = __func__; // CHECK: warning: initialisation of char[] from __func__ is an extension
}
