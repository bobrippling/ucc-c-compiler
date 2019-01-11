// RUN: %check -e %s -target x86_64-linux
// RUN: %check --prefix=darwin -e %s -target x86_64-darwin

f() __attribute((alias("g"))); // CHECK: error: alias "g" doesn't exist (before "f")
int g;

another() __attribute((alias("f"))) // CHECK: error: alias "another" cannot be a definition
{
}

int h __attribute((alias("g"))); // CHECK: error: alias "h" cannot be a definition
// CHECK-darwin: ^error: __attribute__((alias(...))) not supported on this target (for variables)
