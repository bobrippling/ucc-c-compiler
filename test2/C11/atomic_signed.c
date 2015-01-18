// RUN: %check -e %s

unsigned _Atomic int q; // CHECK: !/error/
_Atomic unsigned q2; // CHECK: !/error/

unsigned _Atomic int w; // CHECK: !/error/
signed _Atomic(int) z; // CHECK: error: _Atomic specifier with "signed"
