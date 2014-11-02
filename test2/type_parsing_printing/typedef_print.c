// RUN: %ucc -Xprint -fno-print-typedefs %s | grep -F "int *x()"
// RUN: %ucc -Xprint %s | grep -F "fn (aka 'P ()') *x"

typedef int I;
typedef I *P;
typedef P fn();

fn x;
