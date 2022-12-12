// RUN: %check -e %s -DFAIL
// RUN: %ocheck 0 %s
void abort(void) __attribute__((noreturn));

const char *p = "\xff""f"; // CHECK: !/error/
// ^ the literal is const char[3] holding {'\xff','f','\0'}

#ifdef FAIL
const char *q = "\xfff"; // CHECK: error: escape sequence out of range

const char *r = "\777"; // CHECK: error: escape sequence out of range
const char *s = "\77""7"; // CHECK: !/error/

unsigned char a = '\xfff'; // CHECK: error: escape sequence out of range
unsigned char b = '\xff'; // CHECK: !/error/
#endif

int main()
{
#include "../ocheck-init.c"
	if(p[0] != -1 || p[1] != 'f' || p[2] != '\0')
		abort();
	return 0;
}
