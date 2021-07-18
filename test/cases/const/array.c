// RUN: %layout_check --sections %s

// should be in rodata
int const x[] = { 1, 2, 3 };
