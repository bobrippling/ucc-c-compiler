// RUN: %check -e %s

f() // CHECK: /error: duplicate definitions of "f"/
{
}

f();

f()
{
}
