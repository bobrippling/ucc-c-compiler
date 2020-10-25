// RUN: cp %s ./save-temps.c
// RUN: ! %ucc -save-temps save-temps.c
// RUN: f=%s; test   -f save-temps.i
// RUN: f=%s; test ! -f save-temps.s
// RUN: f=%s; test ! -f save-temps.o
// RUN: rm -f save-temps.[ci]

syntax error
