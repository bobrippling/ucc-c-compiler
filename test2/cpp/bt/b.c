// RUN: %ucc -fsyntax-only %s | grep 'b.c:3: included from here'

#include "a.h"

int main()
{
}
