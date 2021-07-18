// RUN: %ocheck 0 %s

// test &"..."
char (*p)[] = &"";
char (**a)[] = &(char (*)[]){&""};

main()
{
#include "../ocheck-init.c"
	if(p != *a)
		return 1;
	return 0;
}
