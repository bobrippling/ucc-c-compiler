// RUN: ! %ucc -S -o- %s
_Alignas(3) char c; // not ^2
