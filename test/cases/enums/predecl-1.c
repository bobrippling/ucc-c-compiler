// RUN: %check -e %s

enum A;

f(enum A a) // CHECK: /error: function argument "a" has incomplete type 'enum A'/
{
}
