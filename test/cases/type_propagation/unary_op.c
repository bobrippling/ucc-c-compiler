// RUN: %ucc -fsyntax-only %s

a = _Generic((char)1, char: 5);
b = _Generic(+(char)1, int: 8);
