// RUN: %check -e %s -fno-builtin -fsyntax-only

a = __builtin_constant_p(3); // CHECK: error: global scalar initialiser not constant
