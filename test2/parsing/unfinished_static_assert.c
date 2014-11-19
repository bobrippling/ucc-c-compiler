// RUN: %check -e %s

_Static_assert(1,"");

_Static_assert(1, // CHECK: error: expecting token string
