// RUN: %ucc -S -o/dev/null %s 2>&1 | grep 'b.c:3: included from here'

#include "a.h"

int main()
{
}
