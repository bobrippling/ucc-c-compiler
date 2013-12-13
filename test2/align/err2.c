// RUN: ! %ucc -S -o- %s
_Alignas(1) int i; // lowers alignment
