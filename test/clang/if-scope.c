// RUN: %clang_cc1 -fsyntax-only -verify -std=c99 %s

int f (int z)
{ 
   if (z > (int) sizeof (enum {a, b}))
      return a;
   return b; // C99 ? expected-error{{use of undeclared identifier}} : nothing
}
