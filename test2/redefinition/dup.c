// RUN: %check -e %s

f() // CHECK: /note:/
{
}

f();

f() // CHECK: /error: duplicate definitions of "f"/
{
}
