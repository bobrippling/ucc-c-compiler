// RUN: true
#define x
x#define y z
y

// expands to:
// ```
// #define y z
// y
// ```
