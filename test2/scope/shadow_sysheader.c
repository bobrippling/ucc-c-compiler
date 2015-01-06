// RUN: %check --prefix=ALL   %s -Wshadow-global-all -nostdinc -isystem sysheader
// RUN: %check --prefix=USR   %s -Wshadow-global     -nostdinc -isystem sysheader
// RUN: %check --prefix=NOSYS %s -Wshadow-global

# 1 "sysheader/hi.h"

int glob;

# 10 "here.c"
// XXX: ^ remember to update this if adding anything

main()
{
	int glob = 3; // CHECK-ALL: warning: declaration of "glob" shadows global declaration
	// CHECK-USR:   ^  !/warning:.*shadow/
	// CHECK-NOSYS: ^^ warning: declaration of "glob" shadows global declaration

	return glob;
}
