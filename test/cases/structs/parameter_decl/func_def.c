// RUN: %check -e %s

f(struct A *); // CHECK: /note: previous definition/

f(struct A *p) // CHECK: /error: mismatching definitions of "f"/
{
}
