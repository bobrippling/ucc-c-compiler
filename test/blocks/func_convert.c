// RUN: %check --only %s

void f(int (^fn)(int))
{
  int (*p)(int) = fn; // CHECK: warning: mismatching types, initialisation
  // CHECK: ^note: 'int (*)(int)' vs 'int (^)(int)'

  // previously we'd classify a block-pointer as an integer (or more
  // specifically, not a pointer) and emit a pointer <--> integer conversion
  // warning
}
