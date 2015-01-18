// RUN: %check -e %s

_Atomic(int) i; // CHECK: !/error/

_Atomic(const int) j; // CHECK: error: qualified type in _Atomic
_Atomic(_Atomic(int)) k; // CHECK: error: qualified type in _Atomic()
_Atomic(int ()) l; // CHECK: error: function type (int ()) in _Atomic()
_Atomic(int [2]) m; // CHECK: error: array type (int[2]) in _Atomic()
