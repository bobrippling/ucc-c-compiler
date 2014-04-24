// RUN: %check -e %s
int i = _Generic("hi", const char *: 0); // CHECK: error: no type satisfying char *
