// RUN: %ocheck 0 %s -fno-const-fold

enum { A };

main()
{
#include "../ocheck-init.c"
	return A;
}
