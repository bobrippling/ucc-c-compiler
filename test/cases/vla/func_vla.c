// RUN: %check -e %s

int g();

int (*f())[g()] // CHECK: error: function with variably modified type
{
}
