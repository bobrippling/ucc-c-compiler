// RUN: %check -e %s
int _Atomic(char) a; // CHECK: error: _Atomic specifier with previous type (int)
_Atomic(char) int b; // CHECK: error: expecting token ';', got int
