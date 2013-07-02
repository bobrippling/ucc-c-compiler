// RUN: %ucc -c %s

int x __attribute__(( aligned( __alignof__(int) ) ));

_Static_assert(_Alignof(x) == 4, "bad attribute?");
