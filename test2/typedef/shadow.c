// RUN: %check %s -Wshadow

typedef unsigned long size_t; // CHECK: note: global declaration here

main()
{
	typedef void *size_t; // CHECK: warning: declaration of "size_t" shadows global declaration

	size_t p = (void *)5;

	return (int)p;
}
