// RUN: %ucc -fno-builtin %s -fsyntax-only

a = __builtin_constant_p(3);
