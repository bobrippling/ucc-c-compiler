// RUN: %check --only -e %s

__attribute((aligned(31))) // CHECK: error: alignment 31 isn't a power of 2
void f()
{
}
