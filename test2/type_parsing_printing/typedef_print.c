// RUN: %ucc -emit=print -fno-print-typedefs %s | grep -F "x 'int *()'"
// RUN: %ucc -emit=print %s | grep -F "x 'fn (aka 'P ()') *'"

typedef int I;
typedef I *P;
typedef P fn();

fn x;
