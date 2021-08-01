// RUN: %check --only --prefix=c89 %s -std=c89
// RUN: %check --only --prefix=c99 %s -std=c99

typedef const int k;

const const int i; // CHECK-c89: warning: duplicate 'const' specifier
// CHECK-c99: ^ warning: duplicate 'const' specifier

k const ki; // CHECK-c89: warning: duplicate 'const' specifier
const k ki2; // CHECK-c89: warning: duplicate 'const' specifier

__typeof(const int) const q; // CHECK-c89: warning: duplicate 'const' specifier
