#include <stdio.h>
#include <string.h>

//#include <assert.h>
#define assert(x) printf("%d = " #x "\n" , x)

int main()
{
	char *s = "hi there";

	strtest(strcmp("hi", "hi",               == 0);
	strtest(strcmp("yo", "hi",               >  0);
	strtest(strcmp("a",  "b" ,               <  0);
	strtest(strcmp("",   "",                 == 0);

	strtest(strcmp("aaa", "aaz",             <  0);

	strtest(strcmp("aaa", ""   ,             > 0);
	strtest(strcmp("",    "aaa",             < 0);

	strtest(strncmp("hi", "hia", 2,          == 0);

	strtest(strncmp("hello", "henry", 2,     == 0);
	strtest(strncmp("hello", "henry", 3,     <  0);

	strtest(strchr(s, 'i',                   == s + 1);
	strtest(strchr(s, 'z',                   == NULL);
}
