// RUN: ! %ucc -S -o- %s
typedef _Alignas(8) int aligned_int;
