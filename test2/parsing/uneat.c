// RUN: %ucc -fsyntax-only %s
typedef int tint;
tint f(tint(tint)); // checks parsing/unget of types at '('
