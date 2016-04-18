// RUN: %ucc -H -E %s 2>&1 | grep '^\.' >%t
// RUN: grep -F '. cpp/nesting/a.h' %t
// RUN: grep -F '.. cpp/nesting/b1.h' %t
// RUN: grep -F '... cpp/nesting/c.h' %t
// RUN: grep -F '.. cpp/nesting/b2.h' %t

#include "a.h"
