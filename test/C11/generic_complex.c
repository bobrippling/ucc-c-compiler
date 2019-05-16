// RUN: %check --prefix=e -e %s -DS_OR_U=struct
// RUN: %check --only        %s -DS_OR_U=union -Wno-unused-comma

union f;

#define DECAY(x) (0, x)

_Static_assert(
		_Generic(
			DECAY(*(__typeof((*(struct A { int (*i)(union f); } *)0).i))0)
			, char: 5, int (*)(S_OR_U f): 1) // CHECK-e: error: redeclaration of union as struct
		==
		1,
		"");
