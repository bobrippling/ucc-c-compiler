// RUN: %ucc -fsyntax-only %s
f(int a[restrict]);
q(int a[static 10])
{
}
