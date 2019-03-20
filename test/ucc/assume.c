// RUN: %ucc '-###' a -xc b -xnone c -xasm d >%t 2>&1
// RUN:   grep -F 'assuming "a" is object-file' %t
// RUN:   grep    'cpp2/cpp .* b ' %t
// RUN:   grep -F 'assuming "c" is object-file' %t
// RUN: ! grep    'cpp2/cpp .* d ' %t
// RUN:   grep    'as .* d ' %t

// a: assumed, object file
// b: c
// c: assumed, object file
// d: asm
