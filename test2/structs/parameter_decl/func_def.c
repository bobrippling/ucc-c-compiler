// RUN: %check -e %s

f(struct A *); // CHECK: /error: mismatching definitions of "f"/

f(struct A *p) // CHECK: /note: other definition/
{
}
