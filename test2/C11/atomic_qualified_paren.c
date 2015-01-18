// RUN: %check -e %s

_Atomic const int a; // CHECK: !/error/
const _Atomic int b; // CHECK: !/error/

_Atomic(const int) c; // CHECK: error: _Atomic() applied to qualified type
const _Atomic(int) d; // CHECK: !/error/
