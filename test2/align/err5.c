// RUN: %check -e %s

_Alignas(2) f() // CHECK: error: alignment specified for function 'f'
{
}
