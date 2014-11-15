// RUN: %check -e %s

struct A f() // CHECK: error: incomplete return type
{
}
