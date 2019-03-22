// RUN: %ucc '-###' a -xc b -xnone c -xasm d -xasm-with-cpp e >%t 2>&1
// RUN:   grep -F 'assuming "a" is object-file' %t
// RUN:   grep    'cpp2/cpp .* b ' %t
// RUN:   grep -F 'assuming "c" is object-file' %t
// RUN: ! grep    'cpp2/cpp .* d ' %t
// RUN:   grep    'as .* d ' %t
// RUN:   grep    'cpp2/cpp .* -D__ASSEMBLER__=1 .* e ' %t

// a: assumed, object file
// b: c
// c: assumed, object file
// d: asm
// e: asm-with-cpp
