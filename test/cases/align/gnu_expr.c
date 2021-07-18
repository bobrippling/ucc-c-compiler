// RUN: %check %s -Wgnu

int x;
int y = _Alignof(x); // CHECK: warning: _Alignof applied to expression is a GNU extension
int z = _Alignof(int){0}; // CHECK: warning: _Alignof applied to expression is a GNU extension
